#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;

class SoundboardSettings : public QDialog {
	Q_OBJECT

private:
	QCheckBox *monitoringCheckBox = nullptr;
	QCheckBox *hideArtworkCheckBox = nullptr;
	QComboBox *monitoringDeviceComboBox = nullptr;
	QPushButton *refreshButton = nullptr;
	QPushButton *restartButton = nullptr;
	QSpinBox *minimumButtonSizeSpinBox = nullptr;
	QSpinBox *maximumButtonSizeSpinBox = nullptr;
	QComboBox *imagePlacementComboBox = nullptr;
	QComboBox *textPositionComboBox = nullptr;

	void reloadDevices();

public:
	SoundboardSettings(bool monitoringEnabled, bool hideArtwork, int minimumButtonSize, int maximumButtonSize,
			   int imagePlacement, int textPosition, QWidget *parent = nullptr);

	bool monitoringEnabled() const;
	bool hideArtwork() const;
	int minimumButtonSize() const;
	int maximumButtonSize() const;
	int imagePlacement() const;
	int textPosition() const;
	QString deviceName() const;
	QString deviceId() const;
};
