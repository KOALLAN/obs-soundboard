#include "SoundboardSettings.hpp"

#include <obs-module.h>
#include <obs.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#define QTStr(str) QString(obs_module_text(str))

namespace {
bool addMonitoringDevice(void *data, const char *name, const char *id)
{
	QComboBox *comboBox = static_cast<QComboBox *>(data);
	const QString deviceId = QString::fromUtf8(id ? id : "");

	if (comboBox->findData(deviceId) < 0)
		comboBox->addItem(QString::fromUtf8(name ? name : ""), deviceId);

	return true;
}
} // namespace

SoundboardSettings::SoundboardSettings(bool monitoringEnabled_, bool hideArtwork_, QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(QTStr("SoundboardSettings"));
	setMinimumWidth(520);

	monitoringCheckBox = new QCheckBox(QTStr("EnableMonitoring"), this);
	monitoringCheckBox->setChecked(monitoringEnabled_);
	monitoringCheckBox->setToolTip(QTStr("EnableMonitoring.Tooltip"));

	hideArtworkCheckBox = new QCheckBox(QTStr("HideArtwork"), this);
	hideArtworkCheckBox->setChecked(hideArtwork_);
	hideArtworkCheckBox->setToolTip(QTStr("HideArtwork.Tooltip"));

	monitoringDeviceComboBox = new QComboBox(this);
	refreshButton = new QPushButton(QTStr("RefreshDevices"), this);
	restartButton = new QPushButton(QTStr("RestartMonitoring"), this);

	QHBoxLayout *deviceLayout = new QHBoxLayout();
	deviceLayout->addWidget(monitoringDeviceComboBox, 1);
	deviceLayout->addWidget(refreshButton);

	QFormLayout *formLayout = new QFormLayout();
	formLayout->addRow(QString(), monitoringCheckBox);
	formLayout->addRow(QTStr("MonitoringDevice"), deviceLayout);
	formLayout->addRow(QString(), hideArtworkCheckBox);

	QLabel *notice = new QLabel(QTStr("GlobalMonitoringNotice"), this);
	notice->setWordWrap(true);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addWidget(restartButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(buttonBox);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(formLayout);
	mainLayout->addWidget(notice);
	mainLayout->addLayout(buttonLayout);

	connect(refreshButton, &QPushButton::clicked, this, &SoundboardSettings::reloadDevices);
	connect(restartButton, &QPushButton::clicked, this, [this]() {
		obs_reset_audio_monitoring();
		QMessageBox::information(this, QTStr("RestartMonitoring"), QTStr("MonitoringRestarted"));
	});
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	const bool monitoringAvailable = obs_audio_monitoring_available();
	monitoringCheckBox->setEnabled(monitoringAvailable);
	monitoringDeviceComboBox->setEnabled(monitoringAvailable);
	refreshButton->setEnabled(monitoringAvailable);
	restartButton->setEnabled(monitoringAvailable);

	reloadDevices();
}

void SoundboardSettings::reloadDevices()
{
	QString selectedId = monitoringDeviceComboBox->currentData().toString();

	const char *currentName = nullptr;
	const char *currentId = nullptr;
	obs_get_audio_monitoring_device(&currentName, &currentId);

	if (selectedId.isEmpty())
		selectedId = QString::fromUtf8(currentId ? currentId : "default");

	monitoringDeviceComboBox->clear();
	monitoringDeviceComboBox->addItem(QTStr("DefaultMonitoringDevice"), QStringLiteral("default"));
	obs_enum_audio_monitoring_devices(addMonitoringDevice, monitoringDeviceComboBox);

	int index = monitoringDeviceComboBox->findData(selectedId);
	if (index < 0 && currentId && *currentId) {
		monitoringDeviceComboBox->addItem(QString::fromUtf8(currentName ? currentName : currentId),
						  QString::fromUtf8(currentId));
		index = monitoringDeviceComboBox->count() - 1;
	}

	monitoringDeviceComboBox->setCurrentIndex(index >= 0 ? index : 0);
}

bool SoundboardSettings::monitoringEnabled() const
{
	return monitoringCheckBox->isChecked();
}

bool SoundboardSettings::hideArtwork() const
{
	return hideArtworkCheckBox->isChecked();
}

QString SoundboardSettings::deviceName() const
{
	return monitoringDeviceComboBox->currentText();
}

QString SoundboardSettings::deviceId() const
{
	return monitoringDeviceComboBox->currentData().toString();
}
