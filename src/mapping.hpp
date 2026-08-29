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

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <cstdint>

enum class MidiKind : int { ControlChange = 0, ProgramChange = 1, NoteOn = 2 };

enum class ValueMatch : int {
	Any = 0,     // fire on every matching message
	Exact = 1,   // fire only when value == value_exact
	Press = 2,   // fire when value > 0 (CC buttons / notes)
};

struct MidiMatch {
	int channel = 0; // 0 = any channel, 1-16 = specific
	MidiKind kind = MidiKind::ControlChange;
	int number = 0; // CC number, program, or note (0-127)
	ValueMatch value_match = ValueMatch::Press;
	int value_exact = 127;
};

struct HotkeyMapping {
	QString id;
	QString name;
	bool enabled = true;
	MidiMatch midi;
	QString hotkey_name; // obs_hotkey_get_name()
	QString hotkey_label;
	QString hotkey_group; // scene/source name, or "OBS"
};

struct PluginConfig {
	QString device_name;
	int listen_channel = 0; // 0 = any, else 1-16 (additional filter)
	QVector<HotkeyMapping> mappings;

	QJsonObject toJson() const;
	static PluginConfig fromJson(const QJsonObject &obj);
};

QString midiKindName(MidiKind kind);
QString valueMatchName(ValueMatch match);
QString newMappingId();
