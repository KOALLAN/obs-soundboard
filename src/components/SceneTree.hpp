#pragma once

#include <QListWidget>
#include <QObject>
#include <QResizeEvent>
#include <QWidget>

class SceneTree : public QListWidget {
	Q_OBJECT
	Q_PROPERTY(int gridItemWidth READ GetGridItemWidth WRITE SetGridItemWidth DESIGNABLE true)
	Q_PROPERTY(int gridItemHeight READ GetGridItemHeight WRITE SetGridItemHeight DESIGNABLE true)

	bool gridMode = false;
	int maxWidth = 100;
	int renderedWidth = 100;
	int imagePlacement = 0;
	int textPosition = 1;

public:
	enum { PlayingRole = Qt::UserRole + 1 };
	void SetGridMode(bool grid);
	bool GetGridMode() const;
	void SetCardAppearance(int size, int image, int text);
	int GetImagePlacement() const { return imagePlacement; }
	int GetTextPosition() const { return textPosition; }

	void SetGridItemWidth(int width);
	void SetGridItemHeight(int height);
	int GetGridItemWidth() const;
	int GetGridItemHeight() const;
	int GetRenderedGridItemWidth() const { return renderedWidth; }

	explicit SceneTree(QWidget *parent = nullptr);

private:
	void RefreshLayout();
	void UpdateGridSize();

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void startDrag(Qt::DropActions supportedActions) override;
	virtual void dropEvent(QDropEvent *event) override;
	virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
	virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
#endif

signals:
	void scenesReordered();
};
