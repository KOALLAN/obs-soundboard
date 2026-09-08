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
	// Grid positions are calculated explicitly. Letting QListView adjust them
	// again would reintroduce its independent column-wrapping decision.
	setResizeMode(grid ? QListView::Fixed : QListView::Adjust);
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
	gridColumns = 1;
	gridLeft = 0;
	if (gridMode) {
		constexpr int minimumGap = 4;
		const int width = qMax(minWidth + minimumGap, viewport()->contentsRect().width());
		const int columnsByMinimum = qMax(1, width / (minWidth + minimumGap));
		gridColumns = qMin(qMax(1, count()), columnsByMinimum);

		// Grow every card with the dock until the configured maximum is
		// reached. The positions are applied explicitly after QListView has
		// prepared its item rectangles, so its own wrap threshold is bypassed.
		renderedWidth = qBound(minWidth, width / gridColumns - minimumGap, maxWidth);
		const int cellWidth = renderedWidth + minimumGap;
		gridLeft = qMax(0, (width - gridColumns * cellWidth) / 2);
		cell = QSize(cellWidth, cellWidth);
	}
	if (gridSize() != cell)
		setGridSize(cell);
}

void SceneTree::PositionGridItems()
{
	if (!gridMode)
		return;

	const int stepX = qMax(1, gridSize().width());
	const int stepY = qMax(1, gridSize().height());
	for (int i = 0; i < count(); i++) {
		const int column = i % gridColumns;
		const int row = i / gridColumns;
		setPositionForIndex(QPoint(gridLeft + column * stepX, row * stepY), model()->index(i, 0));
	}
}

void SceneTree::RefreshLayout()
{
	if (layoutUpdateInProgress)
		return;
	layoutUpdateInProgress = true;

	// Recalculate responsive card geometry after restores and data changes.
	UpdateGridSize();
	setIconSize(gridMode ? QSize(renderedWidth - 12, renderedWidth - 12) : QSize(32, 32));
	for (int i = 0; i < count(); i++) {
		item(i)->setData(Qt::SizeHintRole, QVariant());
		item(i)->setTextAlignment(gridMode ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignVCenter);
	}
	doItemsLayout();
	PositionGridItems();
	viewport()->update();
	layoutUpdateInProgress = false;
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
	RefreshLayout();
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
		const int columns = gridColumns;
		const QPoint point = event->position().toPoint();
		const int x = qBound(0, (point.x() + horizontalScrollBar()->value() - gridLeft) / stepX,
				     columns - 1);
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
