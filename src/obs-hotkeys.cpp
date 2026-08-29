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

#include "obs-hotkeys.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/threading.h>

#include "plugin-support.h"

namespace {

struct EnumState {
	QVector<ObsHotkeyInfo> list;
};

struct FindState {
	QByteArray name;
	QString group;
	obs_hotkey_id id = OBS_INVALID_HOTKEY_ID;
	obs_hotkey_registerer_t reg = OBS_HOTKEY_REGISTERER_FRONTEND;
	obs_weak_source_t *weak_source = nullptr;
	bool found = false;
};

struct TriggerJob {
	obs_hotkey_id id = OBS_INVALID_HOTKEY_ID;
	QString scene_name;
	obs_weak_source_t *weak_source = nullptr;
	bool is_scene = false;
};

} // namespace

QString hotkeyGroupName(obs_hotkey_t *hotkey)
{
	switch (obs_hotkey_get_registerer_type(hotkey)) {
	case OBS_HOTKEY_REGISTERER_FRONTEND:
		return QStringLiteral("OBS");
	case OBS_HOTKEY_REGISTERER_SOURCE: {
		auto *weak = static_cast<obs_weak_source_t *>(obs_hotkey_get_registerer(hotkey));
		OBSSourceAutoRelease src = obs_weak_source_get_source(weak);
		return src ? QString::fromUtf8(obs_source_get_name(src)) : QStringLiteral("Source");
	}
	case OBS_HOTKEY_REGISTERER_OUTPUT:
		return QStringLiteral("Output");
	case OBS_HOTKEY_REGISTERER_ENCODER:
		return QStringLiteral("Encoder");
	case OBS_HOTKEY_REGISTERER_SERVICE:
		return QStringLiteral("Service");
	default:
		return QStringLiteral("Other");
	}
}

QVector<ObsHotkeyInfo> enumerateObsHotkeys()
{
	EnumState state;
	obs_enum_hotkeys(
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey) -> bool {
			auto *st = static_cast<EnumState *>(data);
			ObsHotkeyInfo info;
			info.id = id;
			info.name = QString::fromUtf8(obs_hotkey_get_name(hotkey));
			info.description = QString::fromUtf8(obs_hotkey_get_description(hotkey));
			info.group = hotkeyGroupName(hotkey);
			st->list.push_back(info);
			return true;
		},
		&state);
	return state.list;
}

static bool setSceneByName(const QString &sceneName)
{
	if (sceneName.isEmpty() || sceneName == QLatin1String("OBS"))
		return false;

	OBSSourceAutoRelease src = obs_get_source_by_name(sceneName.toUtf8().constData());
	if (!src || obs_source_get_type(src) != OBS_SOURCE_TYPE_SCENE)
		return false;

	obs_frontend_set_current_scene(src);
	obs_log(LOG_INFO, "switched scene to '%s'", sceneName.toUtf8().constData());
	return true;
}

static void triggerJob(void *param)
{
	auto *job = static_cast<TriggerJob *>(param);

	if (setSceneByName(job->scene_name)) {
		delete job;
		return;
	}

	if (job->is_scene && job->weak_source) {
		OBSSourceAutoRelease src = obs_weak_source_get_source(job->weak_source);
		if (src && obs_source_get_type(src) == OBS_SOURCE_TYPE_SCENE) {
			obs_frontend_set_current_scene(src);
			delete job;
			return;
		}
	}

	obs_hotkey_enable_background_press(true);
	obs_hotkey_trigger_routed_callback(job->id, false);
	obs_hotkey_trigger_routed_callback(job->id, true);
	obs_hotkey_trigger_routed_callback(job->id, false);
	delete job;
}

bool triggerObsHotkeyByName(const QString &hotkeyName, const QString &group)
{
	if (!group.isEmpty() && group != QLatin1String("OBS")) {
		OBSSourceAutoRelease probe = obs_get_source_by_name(group.toUtf8().constData());
		if (probe && obs_source_get_type(probe) == OBS_SOURCE_TYPE_SCENE) {
			auto *job = new TriggerJob();
			job->scene_name = group;
			job->is_scene = true;
			obs_queue_task(OBS_TASK_UI, triggerJob, job, false);
			return true;
		}
	}

	if (hotkeyName.isEmpty())
		return false;

	FindState st;
	st.name = hotkeyName.toUtf8();
	st.group = group;

	obs_enum_hotkeys(
		[](void *data, obs_hotkey_id id, obs_hotkey_t *hotkey) -> bool {
			auto *s = static_cast<FindState *>(data);
			if (s->name != obs_hotkey_get_name(hotkey))
				return true;

			const QString g = hotkeyGroupName(hotkey);
			if (!s->group.isEmpty() && g != s->group)
				return true;

			s->found = true;
			s->id = id;
			s->reg = obs_hotkey_get_registerer_type(hotkey);
			if (s->reg == OBS_HOTKEY_REGISTERER_SOURCE)
				s->weak_source =
					static_cast<obs_weak_source_t *>(obs_hotkey_get_registerer(hotkey));
			return false;
		},
		&st);

	if (!st.found) {
		obs_log(LOG_WARNING, "hotkey not found: %s [%s]", st.name.constData(),
			group.toUtf8().constData());
		return false;
	}

	auto *job = new TriggerJob();
	job->id = st.id;
	job->scene_name = group;
	job->weak_source = st.weak_source;
	if (st.weak_source) {
		OBSSourceAutoRelease src = obs_weak_source_get_source(st.weak_source);
		job->is_scene = src && obs_source_get_type(src) == OBS_SOURCE_TYPE_SCENE;
	}

	obs_queue_task(OBS_TASK_UI, triggerJob, job, false);
	return true;
}
