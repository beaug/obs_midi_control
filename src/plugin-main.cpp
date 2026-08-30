/*
obs-midi-hotkeys
Inspired by obs-midi-mg
Copyright (C) 2022-2026 nhielost <nhielost@gmail.com>
Copyright (C) 2026 obs-midi-hotkeys contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QAction>
#include <QFile>
#include <QJsonDocument>
#include <QMainWindow>

#include "midi-engine.hpp"
#include "plugin-support.h"
#include "settings-window.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Map MIDI CC / PC / notes to OBS hotkeys.";
}

namespace {

MidiEngine *g_engine = nullptr;
SettingsWindow *g_window = nullptr;
QString g_config_path;

QString configPath()
{
	char *raw = obs_module_get_config_path(obs_current_module(), "mappings.json");
	QString path = QString::fromUtf8(raw ? raw : "");
	bfree(raw);
	return path;
}

void saveConfig(const PluginConfig &cfg)
{
	QFile f(g_config_path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		obs_log(LOG_WARNING, "could not write %s", g_config_path.toUtf8().constData());
		return;
	}
	f.write(QJsonDocument(cfg.toJson()).toJson(QJsonDocument::Indented));
}

PluginConfig loadConfig()
{
	QFile f(g_config_path);
	if (!f.open(QIODevice::ReadOnly))
		return {};
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject())
		return {};
	return PluginConfig::fromJson(doc.object());
}

void openWindow()
{
	if (!g_window)
		return;
	g_window->show();
	g_window->raise();
	g_window->activateWindow();
}

} // namespace

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "loaded %s %s", PLUGIN_NAME, PLUGIN_VERSION);

	char *dir = obs_module_get_config_path(obs_current_module(), "");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	g_config_path = configPath();
	g_engine = new MidiEngine();

	const PluginConfig cfg = loadConfig();
	g_engine->setConfig(cfg);
	if (!cfg.device_name.isEmpty())
		g_engine->open(cfg.device_name);

	auto *obs_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	g_window = new SettingsWindow(g_engine, obs_window);
	g_window->setAttribute(Qt::WA_DeleteOnClose, false);
	g_window->loadFrom(cfg);

	QObject::connect(g_window, &SettingsWindow::configChanged, g_window, [](const PluginConfig &c) {
		saveConfig(c);
	});

	QAction *action = static_cast<QAction *>(
		obs_frontend_add_tools_menu_qaction(obs_module_text("MIDIHotkeys.Menu")));
	QObject::connect(action, &QAction::triggered, openWindow);

	return true;
}

void obs_module_unload(void)
{
	if (g_engine)
		g_engine->close();

	/* Do not delete the dialog here. It is parented to the OBS main
	 * window; Qt is already destroying that tree during shutdown. */
	g_window = nullptr;

	delete g_engine;
	g_engine = nullptr;
	obs_log(LOG_INFO, "unloaded");
}
