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
#include <QSpinBox>
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

SoundboardSettings::SoundboardSettings(bool monitoringEnabled_, bool hideArtwork_, bool automaticCovers_,
				     int minimumButtonSize_, int maximumButtonSize_, int imagePlacement_,
				     int textPosition_, QWidget *parent)
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

	automaticCoversCheckBox = new QCheckBox(QTStr("AutomaticCovers"), this);
	automaticCoversCheckBox->setChecked(automaticCovers_);
	automaticCoversCheckBox->setToolTip(QTStr("AutomaticCovers.Tooltip"));

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

	minimumButtonSizeSpinBox = new QSpinBox(this);
	minimumButtonSizeSpinBox->setRange(64, 256);
	minimumButtonSizeSpinBox->setSuffix(QStringLiteral(" px"));
	minimumButtonSizeSpinBox->setValue(qBound(64, minimumButtonSize_, 256));
	minimumButtonSizeSpinBox->setToolTip(QTStr("MinimumButtonSize.Tooltip"));

	maximumButtonSizeSpinBox = new QSpinBox(this);
	maximumButtonSizeSpinBox->setRange(minimumButtonSizeSpinBox->value(), 256);
	maximumButtonSizeSpinBox->setSuffix(QStringLiteral(" px"));
	maximumButtonSizeSpinBox->setValue(qBound(minimumButtonSizeSpinBox->value(), maximumButtonSize_, 256));
	maximumButtonSizeSpinBox->setToolTip(QTStr("MaximumButtonSize.Tooltip"));
	minimumButtonSizeSpinBox->setMaximum(maximumButtonSizeSpinBox->value());

	imagePlacementComboBox = new QComboBox(this);
	imagePlacementComboBox->addItem(QTStr("ImagePlacement.Top"));
	imagePlacementComboBox->addItem(QTStr("ImagePlacement.Fill"));
	imagePlacementComboBox->setCurrentIndex(imagePlacement_);

	textPositionComboBox = new QComboBox(this);
	textPositionComboBox->addItem(QTStr("TextPosition.Top"));
	textPositionComboBox->addItem(QTStr("TextPosition.Center"));
	textPositionComboBox->addItem(QTStr("TextPosition.Bottom"));
	textPositionComboBox->setCurrentIndex(textPosition_);

	formLayout->addRow(QTStr("MinimumButtonSize"), minimumButtonSizeSpinBox);
	formLayout->addRow(QTStr("MaximumButtonSize"), maximumButtonSizeSpinBox);
	formLayout->addRow(QTStr("ImagePlacement"), imagePlacementComboBox);
	formLayout->addRow(QTStr("TextPosition"), textPositionComboBox);
	formLayout->addRow(QString(), automaticCoversCheckBox);
	QLabel *appearanceNotice = new QLabel(QTStr("CardAppearanceNotice"), this);
	appearanceNotice->setWordWrap(true);
	formLayout->addRow(appearanceNotice);

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
	connect(minimumButtonSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), maximumButtonSizeSpinBox,
		[this](int value) { maximumButtonSizeSpinBox->setMinimum(value); });
	connect(maximumButtonSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), minimumButtonSizeSpinBox,
		[this](int value) { minimumButtonSizeSpinBox->setMaximum(value); });
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

bool SoundboardSettings::automaticCovers() const
{
	return automaticCoversCheckBox->isChecked();
}

QString SoundboardSettings::deviceName() const
{
	return monitoringDeviceComboBox->currentText();
}

int SoundboardSettings::minimumButtonSize() const
{
	return minimumButtonSizeSpinBox->value();
}

int SoundboardSettings::maximumButtonSize() const
{
	return maximumButtonSizeSpinBox->value();
}

int SoundboardSettings::imagePlacement() const
{
	return imagePlacementComboBox->currentIndex();
}

int SoundboardSettings::textPosition() const
{
	return textPositionComboBox->currentIndex();
}

QString SoundboardSettings::deviceId() const
{
	return monitoringDeviceComboBox->currentData().toString();
}
