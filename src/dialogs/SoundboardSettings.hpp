#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QPushButton;

class SoundboardSettings : public QDialog {
	Q_OBJECT

private:
	QCheckBox *monitoringCheckBox = nullptr;
	QCheckBox *hideArtworkCheckBox = nullptr;
	QComboBox *monitoringDeviceComboBox = nullptr;
	QPushButton *refreshButton = nullptr;
	QPushButton *restartButton = nullptr;

	void reloadDevices();

public:
	SoundboardSettings(bool monitoringEnabled, bool hideArtwork, QWidget *parent = nullptr);

	bool monitoringEnabled() const;
	bool hideArtwork() const;
	QString deviceName() const;
	QString deviceId() const;
};
