#include "Soundboard.hpp"
#include "ui_Soundboard.h"

#include <obs-frontend-api.h>
#include <obs-hotkey.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/config-file.h>
#include <util/util.hpp>

#include "plugin-support.h"

#include "components/SceneTree.hpp"
#include "components/MediaControls.hpp"
#include "dialogs/MediaEdit.hpp"
#include "dialogs/SoundboardSettings.hpp"
#include "models/MediaData.hpp"

#include <QAction>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QIcon>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QTimer>

#include "moc_Soundboard.cpp"

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define QTStr(str) QString(obs_module_text(str))
#define MainStr(str) QString(obs_frontend_get_locale_string(str))

namespace {
constexpr const char *HIDE_ARTWORK_FILTER_NAME = "Soundboard - Hide Cover Artwork";

QIcon cardThumbnail(const QString &imagePath)
{
	if (imagePath.isEmpty())
		return {};

	QPixmap original(imagePath);
	if (original.isNull())
		return {};

	// Decode once when adding/editing the sound, not on every repaint.
	return QIcon(original.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QString getDefaultString(QString name = "")
{
	if (name.isEmpty())
		name = QTStr("Sound");

	if (!MediaObj::findByName(name))
		return name;

	int i = 2;

	for (;;) {
		QString out = name + " " + QString::number(i);

		if (!MediaObj::findByName(out))
			return out;

		i++;
	}
}

void onSave(obs_data_t *saveData, bool saving, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);

	if (saving)
		sb->save(saveData);
	else
		sb->load(saveData);
}

void onEvent(enum obs_frontend_event event, void *data)
{
	Soundboard *sb = static_cast<Soundboard *>(data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		sb->createSource();
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		sb->clear();
	default:
		break;
	};
}
} // namespace

Soundboard::Soundboard(QWidget *parent) : QWidget(parent), ui(new Ui_Soundboard)
{
	ui->setupUi(this);
	ui->list->SetGridMode(true);

	for (QAction *x : ui->toolbar->actions()) {
		QWidget *temp = ui->toolbar->widgetForAction(x);

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	obs_frontend_add_event_callback(onEvent, this);
	obs_frontend_add_save_callback(onSave, this);

	ui->list->setItemDelegate(new MediaRenameDelegate(ui->list));

	renameMedia = new QAction(MainStr("Rename"), this);
	renameMedia->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(renameMedia, &QAction::triggered, this, &Soundboard::editMediaName);

#ifdef __APPLE__
	renameMedia->setShortcut({Qt::Key_Return});
#else
	renameMedia->setShortcut({Qt::Key_F2});
#endif

	addAction(renameMedia);

	connect(ui->list->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, &Soundboard::mediaNameEdited);
	// Reuse the existing progress timer to reflect errors as well as playback events.
	connect(&ui->mediaControls->mediaTimer, &QTimer::timeout, this, &Soundboard::updatePlaybackAppearance);
}

Soundboard::~Soundboard()
{
	playbackSignals.clear();
	obs_frontend_remove_event_callback(onEvent, this);
	obs_frontend_remove_save_callback(onSave, this);
}

MediaObj *Soundboard::getCurrentMediaObj()
{
	QListWidgetItem *item = ui->list->currentItem();

	if (!item)
		return nullptr;

	QString uuid = item->data(Qt::UserRole).toString();
	return MediaObj::findByUUID(uuid);
}

QListWidgetItem *Soundboard::findItem(MediaObj *obj)
{
	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		if (uuid == obj->getUUID())
			return item;
	}

	return nullptr;
}

void Soundboard::createSource()
{
	if (obs_obj_invalid(source)) {
		source = obs_source_create("ffmpeg_source", obs_module_text("Soundboard"), nullptr, nullptr);
		configureSource();
	}

	obs_set_output_source(63, source);
}

void Soundboard::configureSource()
{
	playbackSignals.clear();
	if (obs_obj_invalid(source))
		return;

	obs_source_set_hidden(source, true);
	ui->mediaControls->SetSource(source.Get());
	auto playbackChanged = [](void *data, calldata_t *) {
		auto *soundboard = static_cast<Soundboard *>(data);
		// OBS can emit from decoder threads. Read current state on the UI thread
		// so a delayed event from the previous file cannot highlight the wrong card.
		QMetaObject::invokeMethod(soundboard, [soundboard]() { soundboard->updatePlaybackAppearance(); },
					 Qt::QueuedConnection);
	};
	for (const char *event : {"media_started", "media_play", "media_pause", "media_restart", "media_stopped",
				 "media_ended"}) {
		playbackSignals.emplace_back(obs_source_get_signal_handler(source), event, playbackChanged, this);
	}
	updatePlaybackAppearance();

	applyArtworkVisibility();
	initializeMonitoring();
}

void Soundboard::applyArtworkVisibility()
{
	if (obs_obj_invalid(source))
		return;

	OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, HIDE_ARTWORK_FILTER_NAME);
	if (!filter && hideArtwork) {
		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_double(settings, "opacity", 0.0);
		filter = obs_source_create_private("color_filter", HIDE_ARTWORK_FILTER_NAME, settings);

		if (filter)
			obs_source_filter_add(source, filter);
	}

	if (filter)
		obs_source_set_enabled(filter, hideArtwork);
}

void Soundboard::initializeMonitoring()
{
	if (obs_obj_invalid(source))
		return;

	const obs_monitoring_type monitoringType = obs_source_get_monitoring_type(source);
	if (monitoringType == OBS_MONITORING_TYPE_NONE)
		return;

	// Rebuild the route once while the source is idle. Repeating this for every
	// playback can tear down the monitoring output mid-start and create clicks.
	obs_source_set_monitoring_type(source, OBS_MONITORING_TYPE_NONE);
	QTimer::singleShot(0, this, [this, monitoringType]() {
		if (!obs_obj_invalid(source))
			obs_source_set_monitoring_type(source, monitoringType);
	});
}

void Soundboard::updatePlaybackAppearance()
{
	const bool playing = !obs_obj_invalid(source) &&
			     obs_source_media_get_state(source) == OBS_MEDIA_STATE_PLAYING;
	const QString uuid = playing && activeMedia ? activeMedia->getUUID() : QString();
	for (int i = 0; i < ui->list->count(); i++) {
		auto *item = ui->list->item(i);
		const bool active = !uuid.isEmpty() && item->data(Qt::UserRole).toString() == uuid;
		if (item->data(SceneTree::PlayingRole).toBool() != active)
			item->setData(SceneTree::PlayingRole, active);
	}
}

void Soundboard::applyItemAppearance(MediaObj *obj, QListWidgetItem *item)
{
	if (!obj || !item)
		return;

	item->setText(obj->getName());
	item->setToolTip(obj->getName());
	item->setIcon(cardThumbnail(obj->getImagePath()));
	if (ui->list->GetGridMode())
		item->setTextAlignment(Qt::AlignCenter);
	else
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

OBSDataArray Soundboard::saveMedia()
{
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		MediaObj *obj = MediaObj::findByUUID(uuid);

		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_string(settings, "name", QT_TO_UTF8(obj->getName()));
		obs_data_set_string(settings, "path", QT_TO_UTF8(obj->getPath()));
		obs_data_set_bool(settings, "loop", obj->loopEnabled());
		obs_data_set_double(settings, "volume", (double)obj->getVolume());
		obs_data_set_string(settings, "image_path", QT_TO_UTF8(obj->getImagePath()));

		OBSDataArrayAutoRelease hotkeyArray = obs_hotkey_save(obj->getHotkey());
		obs_data_set_array(settings, "sound_hotkey", hotkeyArray);

		obs_data_array_push_back(array, settings);
	};

	return array.Get();
}

void Soundboard::loadMedia(OBSDataArray array)
{
	for (size_t i = 0; i < obs_data_array_count(array); i++) {
		OBSDataAutoRelease settings = obs_data_array_item(array, i);

		obs_data_set_default_string(settings, "name", obs_module_text("Sound"));
		obs_data_set_default_double(settings, "volume", 1.0);
		obs_data_set_default_string(settings, "image_path", "");

		QString name = obs_data_get_string(settings, "name");
		QString path = obs_data_get_string(settings, "path");
		bool loop = obs_data_get_bool(settings, "loop");
		float volume = (float)obs_data_get_double(settings, "volume");
		QString imagePath = obs_data_get_string(settings, "image_path");

		OBSDataArrayAutoRelease hotkeyArray = obs_data_get_array(settings, "sound_hotkey");

		MediaObj *obj = add(name, path, imagePath);
		obs_hotkey_load(obj->getHotkey(), hotkeyArray);
		obj->setLoopEnabled(loop);
		obj->setVolume(volume);
	}
}

void Soundboard::save(OBSData saveData)
{
	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	QDockWidget *dock = static_cast<QDockWidget *>(parent());

	OBSDataArray array = saveMedia();
	obs_data_set_array(saveData, "soundboard_array", array);

	if (!obs_obj_invalid(source)) {
		OBSDataAutoRelease sourceData = obs_save_source(source);
		obs_data_set_obj(saveData, "soundboard_source", sourceData);
	}

	obs_data_set_bool(saveData, "dock_visible", dock->isVisible());
	obs_data_set_bool(saveData, "dock_floating", dock->isFloating());
	obs_data_set_string(saveData, "dock_geometry", dock->saveGeometry().toBase64().constData());
	obs_data_set_int(saveData, "dock_area", window->dockWidgetArea(dock));
	obs_data_set_bool(saveData, "grid_mode", ui->list->GetGridMode());
	obs_data_set_bool(saveData, "hide_artwork", hideArtwork);
	obs_data_set_int(saveData, "button_size", ui->list->GetGridItemWidth());
	obs_data_set_int(saveData, "button_image_placement", ui->list->GetImagePlacement());
	obs_data_set_int(saveData, "button_text_position", ui->list->GetTextPosition());

	MediaObj *obj = getCurrentMediaObj();

	if (obj)
		obs_data_set_string(saveData, "current_sound", QT_TO_UTF8(obj->getName()));

	obs_data_set_bool(saveData, "use_countdown", ui->mediaControls->countDownTimer);
}

void Soundboard::loadSource(OBSData saveData)
{
	OBSDataAutoRelease sourceData = obs_data_get_obj(saveData, "soundboard_source");

	if (sourceData) {
		obs_data_set_obj(sourceData, "settings", nullptr);
		source = obs_load_source(sourceData);

		if (obs_obj_invalid(source))
			return;

		configureSource();
	}
}

void Soundboard::load(OBSData saveData)
{
	QMainWindow *window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	QDockWidget *dock = window->findChild<QDockWidget *>("SoundboardDock");

	obs_data_set_default_bool(saveData, "hide_artwork", true);
	hideArtwork = obs_data_get_bool(saveData, "hide_artwork");
	obs_data_set_default_int(saveData, "button_size", 100);
	obs_data_set_default_int(saveData, "button_image_placement", 0);
	obs_data_set_default_int(saveData, "button_text_position", 1);
	ui->list->SetCardAppearance((int)obs_data_get_int(saveData, "button_size"),
				    (int)obs_data_get_int(saveData, "button_image_placement"),
				    (int)obs_data_get_int(saveData, "button_text_position"));

	loadSource(saveData);

	OBSDataArrayAutoRelease array = obs_data_get_array(saveData, "soundboard_array");
	loadMedia(array.Get());

	const char *geometry = obs_data_get_string(saveData, "dock_geometry");

	if (geometry && *geometry)
		dock->restoreGeometry(QByteArray::fromBase64(QByteArray(geometry)));

	const auto dockArea = static_cast<Qt::DockWidgetArea>(obs_data_get_int(saveData, "dock_area"));

	if (dockArea)
		window->addDockWidget(dockArea, dock);

	bool visible = obs_data_get_bool(saveData, "dock_visible");
	dock->setVisible(visible);

	obs_data_set_default_bool(saveData, "dock_floating", true);
	bool floating = obs_data_get_bool(saveData, "dock_floating");
	dock->setFloating(floating);

	obs_data_set_default_bool(saveData, "grid_mode", true);
	bool grid = obs_data_get_bool(saveData, "grid_mode");
	ui->list->SetGridMode(grid);

	QString lastSound = obs_data_get_string(saveData, "current_sound");

	if (!lastSound.isEmpty()) {
		MediaObj *obj = MediaObj::findByName(lastSound);
		QListWidgetItem *item = findItem(obj);

		if (item)
			ui->list->setCurrentItem(item);
		else
			ui->list->setCurrentRow(0);
	} else {
		ui->list->setCurrentRow(0);
	}

	bool countdown = obs_data_get_bool(saveData, "use_countdown");
	ui->mediaControls->countDownTimer = countdown;
}

void Soundboard::clear()
{
	playbackSignals.clear();
	activeMedia = nullptr;
	ui->mediaControls->countDownTimer = false;
	ui->mediaControls->SetSource(nullptr);
	source = nullptr;

	prevPath = "";

	for (int i = 0; i < ui->list->count(); i++) {
		QListWidgetItem *item = ui->list->item(i);
		QString uuid = item->data(Qt::UserRole).toString();

		MediaObj *obj = MediaObj::findByUUID(uuid);
		delete obj;
		obj = nullptr;
	}

	ui->list->clear();

	updateActions();
}

void Soundboard::play(MediaObj *obj)
{
	if (!obj || obs_obj_invalid(source))
		return;

	QString path = obj->getPath();
	QListWidgetItem *item = findItem(obj);
	activeMedia = obj;
	obs_source_set_volume(source, obj->getVolume());
	ui->list->setCurrentItem(item);

	if (prevPath == path) {
		obs_source_media_restart(source);
		updatePlaybackAppearance();
		return;
	}

	prevPath = path;

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "looping", obj->loopEnabled());
	obs_data_set_string(settings, "local_file", QT_TO_UTF8(path));
	obs_data_set_bool(settings, "is_local_file", true);
	obs_source_update(source, settings);
	updatePlaybackAppearance();
}

void Soundboard::itemRenamed(MediaObj *obj)
{
	QListWidgetItem *item = findItem(obj);

	if (item)
		applyItemAppearance(obj, item);
}

MediaObj *Soundboard::add(const QString &name_, const QString &path, const QString &imagePath)
{
	QString name = getDefaultString(name_);

	MediaObj *obj = new MediaObj(name, path);
	obj->setImagePath(imagePath);

	QListWidgetItem *item = new QListWidgetItem(name);
	item->setData(Qt::UserRole, obj->getUUID());
	applyItemAppearance(obj, item);
	ui->list->addItem(item);
	ui->list->setCurrentItem(item);

	connect(obj, &MediaObj::hotkeyPressed, this, &Soundboard::play);
	connect(obj, &MediaObj::renamed, this, &Soundboard::itemRenamed);

	updateActions();

	return obj;
}

void Soundboard::on_actionAdd_triggered()
{
	MediaEdit edit(this);

	auto added = [&, this]() {
		QString name = edit.getName();
		QString path = edit.getPath();
		bool loop = edit.loopChecked();
		float volume = edit.getVolume();
		QString imagePath = edit.getImagePath();

		MediaObj *obj = add(name, path, imagePath);
		obj->setLoopEnabled(loop);
		obj->setVolume(volume);
	};

	connect(&edit, &QDialog::accepted, this, added);

	edit.setName(getDefaultString());
	edit.exec();
}

void Soundboard::on_actionEdit_triggered()
{
	MediaObj *obj = getCurrentMediaObj();

	if (!obj)
		return;

	MediaEdit edit(this);

	auto edited = [&]() {
		QString name = edit.getName();
		QString path = edit.getPath();
		bool loop = edit.loopChecked();
		float volume = edit.getVolume();
		QString imagePath = edit.getImagePath();

		obj->setName(name);
		obj->setPath(path);
		obj->setLoopEnabled(loop);
		obj->setVolume(volume);
		obj->setImagePath(imagePath);
		applyItemAppearance(obj, findItem(obj));
	};

	connect(&edit, &QDialog::accepted, this, edited);

	edit.setName(obj->getName());
	edit.setPath(obj->getPath());
	edit.setLoopChecked(obj->loopEnabled());
	edit.setVolume(obj->getVolume());
	edit.setImagePath(obj->getImagePath());
	edit.exec();

	prevPath = "";
}

void Soundboard::on_list_itemClicked()
{
	play(getCurrentMediaObj());
}

void Soundboard::updateActions()
{
	bool enable = ui->list->count() > 0;

	if (actionsEnabled == enable)
		return;

	ui->actionRemove->setEnabled(enable);
	ui->actionEdit->setEnabled(enable);

	for (QAction *action : ui->toolbar->actions()) {
		QWidget *widget = ui->toolbar->widgetForAction(action);

		if (!widget)
			continue;

		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}

	actionsEnabled = enable;
}

void Soundboard::on_actionRemove_triggered()
{
	MediaObj *obj = getCurrentMediaObj();

	if (!obj)
		return;

	QString name = obj->getName();

	QMessageBox::StandardButton reply = QMessageBox::question(this, MainStr("ConfirmRemove.Title"),
								  MainStr("ConfirmRemove.Text").arg(name),
								  QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::No)
		return;

	QListWidgetItem *item = findItem(obj);
	delete ui->list->takeItem(ui->list->row(item));
	obj->deleteLater();

	updateActions();
}

void Soundboard::on_actionDuplicate_triggered()
{
	MediaObj *obj = getCurrentMediaObj();

	if (!obj)
		return;

	QString name = getDefaultString(obj->getName());
	QString path = obj->getPath();
	bool loop = obj->loopEnabled();
	float volume = obj->getVolume();
	QString imagePath = obj->getImagePath();
	MediaObj *newObj = add(name, path, imagePath);
	newObj->setLoopEnabled(loop);
	newObj->setVolume(volume);
}

void Soundboard::on_actionSettings_triggered()
{
	if (obs_obj_invalid(source))
		createSource();

	const bool monitoringEnabled =
		obs_source_get_monitoring_type(source) != OBS_MONITORING_TYPE_NONE;
	SoundboardSettings settings(monitoringEnabled, hideArtwork, ui->list->GetGridItemWidth(),
				    ui->list->GetImagePlacement(), ui->list->GetTextPosition(), this);

	if (settings.exec() != QDialog::Accepted)
		return;

	const QByteArray deviceName = settings.deviceName().toUtf8();
	const QByteArray deviceId = settings.deviceId().toUtf8();
	const char *currentDeviceName = nullptr;
	const char *currentDeviceId = nullptr;
	obs_get_audio_monitoring_device(&currentDeviceName, &currentDeviceId);

	if (deviceId != QByteArray(currentDeviceId ? currentDeviceId : "default") &&
	    !obs_set_audio_monitoring_device(deviceName.constData(), deviceId.constData())) {
		QMessageBox::warning(this, QTStr("SoundboardSettings"), QTStr("MonitoringDeviceError"));
	} else {
		config_t *profileConfig = obs_frontend_get_profile_config();
		if (profileConfig) {
			config_set_string(profileConfig, "Audio", "MonitoringDeviceName", deviceName.constData());
			config_set_string(profileConfig, "Audio", "MonitoringDeviceId", deviceId.constData());
			config_save_safe(profileConfig, "tmp", nullptr);
		}
	}

	if (settings.monitoringEnabled() != monitoringEnabled)
		obs_source_set_monitoring_type(source, settings.monitoringEnabled()
							  ? OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT
							  : OBS_MONITORING_TYPE_NONE);

	hideArtwork = settings.hideArtwork();
	applyArtworkVisibility();
	ui->list->SetCardAppearance(settings.buttonSize(), settings.imagePlacement(), settings.textPosition());
	obs_frontend_save();
}

void Soundboard::on_list_customContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = ui->list->itemAt(pos);

	QMenu popup(this);

	popup.addAction(ui->actionAdd);
	popup.addAction(ui->actionSettings);
	popup.addAction(MainStr("Basic.Filters"), this, [this]() { obs_frontend_open_source_filters(source); });
	popup.addSeparator();

	if (item) {
		popup.addAction(renameMedia);
		popup.addSeparator();
		popup.addAction(ui->actionEdit);
		popup.addAction(ui->actionRemove);
		popup.addAction(ui->actionDuplicate);
		popup.addSeparator();
	}

	popup.addSeparator();

	QMenu subMenu(MainStr("Basic.Main.ListMode"));
	QAction *listAction = subMenu.addAction(MainStr("List"), this, [this]() { ui->list->SetGridMode(false); });
	listAction->setCheckable(true);
	QAction *gridAction = subMenu.addAction(MainStr("Grid"), this, [this]() { ui->list->SetGridMode(true); });
	gridAction->setCheckable(true);

	bool grid = ui->list->GetGridMode();

	if (grid)
		gridAction->setChecked(true);
	else
		listAction->setChecked(true);

	popup.addMenu(&subMenu);

	popup.exec(QCursor::pos());
}

void Soundboard::dragEnterEvent(QDragEnterEvent *event)
{
	// refuse drops of our own widgets
	if (event->source() != nullptr) {
		event->setDropAction(Qt::IgnoreAction);
		return;
	}

	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void Soundboard::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void Soundboard::dragMoveEvent(QDragMoveEvent *event)
{
	event->acceptProposedAction();
}

void Soundboard::dropEvent(QDropEvent *event)
{
	QStringList supportedExt;
	supportedExt << "mp3"
		     << "aac"
		     << "ogg"
		     << "wav"
		     << "flac";

	foreach(const QUrl &url, event->mimeData()->urls())
	{
		QString path = url.toLocalFile();
		QFileInfo fi(path);
		QString name = fi.completeBaseName();
		QString ext = fi.suffix();

		if (!supportedExt.contains(ext))
			continue;

		add(name, path);
	}
}

void Soundboard::editMediaName()
{
	removeAction(renameMedia);
	QListWidgetItem *item = ui->list->currentItem();
	Qt::ItemFlags flags = item->flags();

	item->setFlags(flags | Qt::ItemIsEditable);
	ui->list->editItem(item);
	item->setFlags(flags);
}

void Soundboard::mediaNameEdited(QWidget *editor)
{
	addAction(renameMedia);
	MediaObj *obj = getCurrentMediaObj();
	QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
	QString name = edit->text().trimmed();

	if (!obj)
		return;

	QListWidgetItem *item = findItem(obj);
	QString origName = obj->getName();

	if (name == origName) {
		item->setText(origName);
		return;
	}

	if (name.isEmpty()) {
		item->setText(origName);
		QMessageBox::warning(this, MainStr("EmptyName.Title"), MainStr("EmptyName.Text"));
		return;
	}

	if (MediaObj::findByName(name)) {
		item->setText(origName);
		QMessageBox::warning(this, MainStr("NameExists.Title"), MainStr("NameExists.Text"));
		return;
	}

	obj->setName(name);
}

MediaRenameDelegate::MediaRenameDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize MediaRenameDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const auto *tree = qobject_cast<SceneTree *>(parent());
	if (tree && tree->GetGridMode()) {
		return tree->gridSize();
	}
	return QStyledItemDelegate::sizeHint(option, index);
}

void MediaRenameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const auto *tree = qobject_cast<SceneTree *>(parent());
	if (!tree || !tree->GetGridMode()) {
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);
	const int side = tree->GetGridItemWidth();
	const QRect card(opt.rect.topLeft() + QPoint(qMax(2, (opt.rect.width() - side) / 2), 2), QSize(side, side));
	const QColor border = index.data(SceneTree::PlayingRole).toBool() ? QColor("#22C55E") : QColor("#2B2E38");
	constexpr qreal radius = 8.0;
	const bool selected = opt.state & QStyle::State_Selected;
	const bool hovered = opt.state & QStyle::State_MouseOver;
	const bool fill = tree->GetImagePlacement() == 1;
	const QPixmap image = opt.icon.pixmap(QSize(512, 512));
	const bool hasImage = !image.isNull();
	QColor background = opt.palette.color(selected ? QPalette::Highlight : QPalette::Button);
	if (hovered && !selected)
		background = background.lighter(115);

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	QPainterPath outline;
	outline.addRoundedRect(QRectF(card), radius, radius);
	painter->setClipPath(outline, Qt::IntersectClip);
	painter->fillRect(card, background);
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->setFont(opt.font);
	QRect textArea = card.adjusted(6, 6, -6, -6);

	if (hasImage) {
		QRect imageArea = fill ? card : card.adjusted(6, 6, -6, -6);
		if (!fill) {
			imageArea.setHeight(card.height() * 3 / 5 - 6);
			textArea.setTop(imageArea.bottom() + 5);
		}
		QRectF source(image.rect());
		QRectF target(imageArea);
		if (fill) {
			// Center-crop only in fill mode; never stretch the image.
			const qreal scale = qMax(target.width() / source.width(), target.height() / source.height());
			const QSizeF crop(target.width() / scale, target.height() / scale);
			source = QRectF(source.center() - QPointF(crop.width() / 2, crop.height() / 2), crop);
		} else {
			const QSize fitted = image.size().scaled(imageArea.size(), Qt::KeepAspectRatio);
			target = QRectF(QPointF(imageArea.center()) - QPointF(fitted.width() / 2.0, fitted.height() / 2.0),
					QSizeF(fitted));
		}
		painter->drawPixmap(target, image, source);
	}

	const int flags = Qt::AlignHCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere;
	const int textHeight = qMin(textArea.height(), opt.fontMetrics.boundingRect(textArea, flags, opt.text).height());
	QRect label = textArea;
	label.setHeight(textHeight);
	if (tree->GetTextPosition() == 1)
		label.moveTop(textArea.top() + (textArea.height() - textHeight) / 2);
	else if (tree->GetTextPosition() == 2)
		label.moveBottom(textArea.bottom());

	if (hasImage && fill) {
		painter->fillRect(label.adjusted(-3, -2, 3, 2), QColor(0, 0, 0, 170));
		painter->setPen(Qt::white);
	} else {
		painter->setPen(opt.palette.color(selected ? QPalette::HighlightedText : QPalette::ButtonText));
	}
	painter->save();
	painter->setClipRect(label, Qt::IntersectClip);
	painter->drawText(label, flags | Qt::AlignTop, opt.text);
	painter->restore();

	// Playback, not selection, determines the border color.
	painter->setBrush(Qt::NoBrush);
	painter->setPen(QPen(border, 2));
	painter->drawRoundedRect(QRectF(card).adjusted(1, 1, -1, -1), radius - 1, radius - 1);
	painter->restore();
}

void MediaRenameDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
					     const QModelIndex &index) const
{
	const auto *tree = qobject_cast<SceneTree *>(parent());
	if (!tree || !tree->GetGridMode()) {
		QStyledItemDelegate::updateEditorGeometry(editor, option, index);
		return;
	}
	QRect rect = option.rect.adjusted(6, 6, -6, -6);
	rect.setLeft(option.rect.left() + qMax(2, (option.rect.width() - tree->GetGridItemWidth()) / 2) + 6);
	rect.setWidth(tree->GetGridItemWidth() - 12);
	const int height = qMin(rect.height(), editor->sizeHint().height());
	rect.setTop(rect.center().y() - height / 2);
	rect.setHeight(height);
	editor->setGeometry(rect);
}

void MediaRenameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QStyledItemDelegate::setEditorData(editor, index);
	QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
	if (lineEdit)
		lineEdit->selectAll();
}

bool MediaRenameDelegate::eventFilter(QObject *editor, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Escape: {
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
			if (lineEdit)
				lineEdit->undo();
			break;
		}
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(editor, event);
}

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("cg2121 (original), KOALLAN (community adjustments)");
OBS_MODULE_USE_DEFAULT_LOCALE("Soundboard", "en-US")

bool obs_module_load(void)
{
	blog(LOG_INFO, "Soundboard plugin version %s is loaded", PLUGIN_VERSION);

	return true;
}

void obs_module_post_load(void)
{
	obs_frontend_push_ui_translation(obs_module_get_string);

	Soundboard *sb = new Soundboard();
	obs_frontend_add_dock_by_id("SoundboardDock", obs_module_text("Soundboard"), sb);

	obs_frontend_pop_ui_translation();
}

void obs_module_unload(void) {}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("Soundboard");
}
