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
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(grid ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	setStyleSheet("");
	RefreshLayout();
}

bool SceneTree::GetGridMode() const
{
	return gridMode;
}

void SceneTree::SetCardAppearance(int size, int image, int text)
{
	maxWidth = qBound(64, size, 256);
	imagePlacement = qBound(0, image, 1);
	textPosition = qBound(0, text, 2);
	RefreshLayout();
}

void SceneTree::SetGridItemWidth(int width)
{
	SetCardAppearance(width, imagePlacement, textPosition);
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

void SceneTree::RefreshLayout()
{
	// Fixed logical pixels: restoring the dock must never resize the cards.
	const QSize cell(maxWidth + 4, maxWidth + 4);
	setGridSize(gridMode ? cell : QSize());
	setIconSize(gridMode ? QSize(maxWidth - 12, maxWidth - 12) : QSize(32, 32));
	for (int i = 0; i < count(); i++) {
		item(i)->setData(Qt::SizeHintRole, gridMode ? QVariant(cell) : QVariant());
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
	// Qt handles wrapping and scroll bars using the same fixed cell size.
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
		const int step = maxWidth + 4;
		const int columns = qMax(1, viewport()->width() / step);
		const QPoint point = event->position().toPoint();
		const int x = qBound(0, (point.x() + horizontalScrollBar()->value()) / step, columns - 1);
		const int y = qMax(0, (point.y() + verticalScrollBar()->value()) / step);
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
