#include "MediaEdit.hpp"
#include "ui_MediaEdit.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "models/MediaData.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSlider>
#include <QSpinBox>
#include <QtMath>

#include <algorithm>

#include "moc_MediaEdit.cpp"

#define QTStr(str) QString(obs_module_text(str))
#define MainStr(str) QString(obs_frontend_get_locale_string(str))

MediaEdit::MediaEdit(QWidget *parent) : QDialog(parent), ui(new Ui_MediaEdit)
{
	obs_frontend_push_ui_translation(obs_module_get_string);
	ui->setupUi(this);
	obs_frontend_pop_ui_translation();

	connect(ui->volumeSlider, &QSlider::valueChanged, ui->volumeSpinBox, &QSpinBox::setValue);
	connect(ui->volumeSpinBox, qOverload<int>(&QSpinBox::valueChanged), ui->volumeSlider, &QSlider::setValue);
}

MediaEdit::~MediaEdit() {}

void MediaEdit::setName(const QString &name)
{
	ui->name->setText(name);

	if (origText.isNull())
		origText = name;
}

QString MediaEdit::getName()
{
	return ui->name->text();
}

void MediaEdit::setPath(const QString &path)
{
	ui->path->setText(path);
}

QString MediaEdit::getPath()
{
	return ui->path->text();
}

void MediaEdit::setLoopChecked(bool checked)
{
	ui->loop->setChecked(checked);
}

bool MediaEdit::loopChecked()
{
	return ui->loop->isChecked();
}

void MediaEdit::setVolume(float volume)
{
	const int percent = std::clamp(qRound(volume * 100.0f), 0, 150);
	ui->volumeSpinBox->setValue(percent);
}

float MediaEdit::getVolume()
{
	return static_cast<float>(ui->volumeSpinBox->value()) / 100.0f;
}

void MediaEdit::on_browseButton_clicked()
{
	QString folder = ui->path->text();

	if (!folder.isEmpty())
		folder = QFileInfo(folder).absoluteDir().absolutePath();
	else
		folder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

	QString fileName = QFileDialog::getOpenFileName(this, QTStr("OpenAudioFile"), folder,
							("Audio (*.mp3 *.aac *.ogg *.wav *.flac)"));

	if (!fileName.isEmpty())
		ui->path->setText(fileName);
}

void MediaEdit::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::RejectRole) {
		close();
	} else if (val == QDialogButtonBox::AcceptRole) {
		QString name = getName();

		if (name.isEmpty()) {
			QMessageBox::warning(this, MainStr("EmptyName.Title"), MainStr("EmptyName.Text"));
			return;
		}

		if (origText != name && MediaObj::findByName(name)) {
			QMessageBox::warning(this, MainStr("NameExists.Title"), MainStr("NameExists.Text"));
			return;
		}

		if (ui->path->text().isEmpty()) {
			QMessageBox::warning(this, QTStr("NoFile.Title"), QTStr("NoFile.Text"));
			return;
		}

		accept();
		close();
	}
}
