#include "SceneTree.hpp"

#include <QDrag>
#include <QMimeData>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>

#include "moc_SceneTree.cpp"

SceneTree::SceneTree(QWidget *parent_) : QListWidget(parent_)
{
	setDragDropMode(InternalMove);
	setMovement(QListView::Snap);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	connect(model(), &QAbstractItemModel::rowsRemoved, this,
		[this]() { QTimer::singleShot(0, this, [this]() { RefreshLayout(); }); });
}

void SceneTree::SetGridMode(bool grid)
{
	parent()->setProperty("class", grid ? "list-grid" : "");
	gridMode = grid;
	setViewMode(grid ? QListView::IconMode : QListView::ListMode);
	setResizeMode(QListView::Adjust);
	setMovement(QListView::Snap);
	setUniformItemSizes(true);
	setWordWrap(grid);
	setSpacing(0);
	setFlow(QListView::LeftToRight);
	setWrapping(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setStyleSheet("");
	RefreshLayout();
}

bool SceneTree::GetGridMode() const
{
	return gridMode;
}

void SceneTree::SetCardAppearance(int minimumSize, int maximumSize, int image, int text)
{
	minWidth = qBound(64, minimumSize, 256);
	maxWidth = qBound(minWidth, maximumSize, 256);
	imagePlacement = qBound(0, image, 1);
	textPosition = qBound(0, text, 2);
	RefreshLayout();
}

void SceneTree::SetGridItemWidth(int width)
{
	SetCardAppearance(qMin(minWidth, width), width, imagePlacement, textPosition);
}

void SceneTree::SetGridItemHeight(int height)
{
	SetGridItemWidth(height);
}

int SceneTree::GetGridItemWidth() const
{
	return maxWidth;
}

int SceneTree::GetGridItemHeight() const
{
	return maxWidth;
}

void SceneTree::UpdateGridSize()
{
	QSize cell;
	renderedWidth = maxWidth;
	if (gridMode) {
		constexpr int minimumGap = 4;
		constexpr int layoutSafety = 1;
		const QMargins oldMargins = viewportMargins();
		const int width = qMax(minWidth + minimumGap + layoutSafety,
				       viewport()->contentsRect().width() + oldMargins.left() + oldMargins.right());
		const int columnsByMinimum = qMax(1, (width - layoutSafety) / (minWidth + minimumGap));
		const int columns = qMin(qMax(1, count()), columnsByMinimum);

		// Grow every card with the dock until the configured maximum is
		// reached. A one-pixel safety allowance prevents QListView from
		// wrapping the final column at exact DPI-scaled boundaries.
		renderedWidth = qBound(minWidth, (width - layoutSafety) / columns - minimumGap, maxWidth);
		const int cellWidth = renderedWidth + minimumGap;
		const int contentWidth = columns * cellWidth + layoutSafety;
		const int spareWidth = qMax(0, width - contentWidth);
		const int leftMargin = spareWidth / 2;
		const int rightMargin = spareWidth - leftMargin;
		if (oldMargins.left() != leftMargin || oldMargins.right() != rightMargin || oldMargins.top() != 0 ||
		    oldMargins.bottom() != 0)
			setViewportMargins(leftMargin, 0, rightMargin, 0);

		cell = QSize(cellWidth, cellWidth);
	} else if (viewportMargins() != QMargins()) {
		setViewportMargins(0, 0, 0, 0);
	}
	if (gridSize() != cell)
		setGridSize(cell);
}

void SceneTree::RefreshLayout()
{
	// Recalculate responsive card geometry after restores and data changes.
	UpdateGridSize();
	setIconSize(gridMode ? QSize(renderedWidth - 12, renderedWidth - 12) : QSize(32, 32));
	for (int i = 0; i < count(); i++) {
		item(i)->setData(Qt::SizeHintRole, QVariant());
		item(i)->setTextAlignment(gridMode ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignVCenter);
	}
	doItemsLayout();
	viewport()->update();
}

void SceneTree::showEvent(QShowEvent *event)
{
	QListWidget::showEvent(event);
	RefreshLayout();
	// OBS restores dock geometry after creating its child widgets.
	QTimer::singleShot(0, this, [this]() { RefreshLayout(); });
}

void SceneTree::resizeEvent(QResizeEvent *event)
{
	QListWidget::resizeEvent(event);
	// Resize cards within their configured range and keep the grid centered.
	UpdateGridSize();
	scheduleDelayedItemsLayout();
}

void SceneTree::startDrag(Qt::DropActions supportedActions)
{
	if (!gridMode) {
		QListWidget::startDrag(supportedActions);
		return;
	}
	if (selectedIndexes().isEmpty())
		return;

	// Grid dropEvent reorders the model itself. Do not let the base startDrag
	// remove the source row a second time after a successful move.
	QDrag *drag = new QDrag(this);
	drag->setMimeData(model()->mimeData(selectedIndexes()));
	drag->exec(Qt::MoveAction);
	drag->deleteLater();
	RefreshLayout();
}

void SceneTree::dropEvent(QDropEvent *event)
{
	if (event->source() != this) {
		QListWidget::dropEvent(event);
		return;
	}

	if (gridMode && !selectedIndexes().isEmpty()) {
		const int stepX = qMax(1, gridSize().width());
		const int stepY = qMax(1, gridSize().height());
		const int columns = qMax(1, viewport()->width() / stepX);
		const QPoint point = event->position().toPoint();
		const int x = qBound(0, (point.x() + horizontalScrollBar()->value()) / stepX, columns - 1);
		const int y = qMax(0, (point.y() + verticalScrollBar()->value()) / stepY);
		const int row = qBound(0, x + y * columns, count() - 1);
		QListWidgetItem *moved = takeItem(selectedIndexes().front().row());
		insertItem(row, moved);
		setCurrentItem(moved);
		event->setDropAction(Qt::MoveAction);
		event->accept();
	} else {
		QListWidget::dropEvent(event);
	}

	RefreshLayout();
	emit scenesReordered();
}

void SceneTree::rowsInserted(const QModelIndex &parent, int start, int end)
{
	QListWidget::rowsInserted(parent, start, end);
	RefreshLayout();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
// Workaround for QTBUG-105870. Remove once that is solved upstream.
void SceneTree::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	if (selected.count() == 0 && deselected.count() > 0 && !property("clearing").toBool())
		setCurrentRow(deselected.indexes().front().row());
}
#endif
