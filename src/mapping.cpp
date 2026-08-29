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

#include "mapping.hpp"

#include <QUuid>

QString midiKindName(MidiKind kind)
{
	switch (kind) {
	case MidiKind::ControlChange:
		return QStringLiteral("CC");
	case MidiKind::ProgramChange:
		return QStringLiteral("PC");
	case MidiKind::NoteOn:
		return QStringLiteral("Note");
	}
	return QStringLiteral("CC");
}

QString valueMatchName(ValueMatch match)
{
	switch (match) {
	case ValueMatch::Any:
		return QStringLiteral("Any");
	case ValueMatch::Exact:
		return QStringLiteral("Exact");
	case ValueMatch::Press:
		return QStringLiteral("Press");
	}
	return QStringLiteral("Press");
}

QString newMappingId()
{
	return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

static QJsonObject matchToJson(const MidiMatch &m)
{
	QJsonObject o;
	o["channel"] = m.channel;
	o["kind"] = static_cast<int>(m.kind);
	o["number"] = m.number;
	o["value_match"] = static_cast<int>(m.value_match);
	o["value_exact"] = m.value_exact;
	return o;
}

static MidiMatch matchFromJson(const QJsonObject &o)
{
	MidiMatch m;
	m.channel = o.value("channel").toInt(0);
	m.kind = static_cast<MidiKind>(o.value("kind").toInt(0));
	m.number = o.value("number").toInt(0);
	m.value_match = static_cast<ValueMatch>(o.value("value_match").toInt(2));
	m.value_exact = o.value("value_exact").toInt(127);
	return m;
}

QJsonObject PluginConfig::toJson() const
{
	QJsonObject root;
	root["device_name"] = device_name;
	root["listen_channel"] = listen_channel;
	QJsonArray arr;
	for (const auto &map : mappings) {
		QJsonObject o;
		o["id"] = map.id;
		o["name"] = map.name;
		o["enabled"] = map.enabled;
		o["midi"] = matchToJson(map.midi);
		o["hotkey_name"] = map.hotkey_name;
		o["hotkey_label"] = map.hotkey_label;
		o["hotkey_group"] = map.hotkey_group;
		arr.append(o);
	}
	root["mappings"] = arr;
	return root;
}

PluginConfig PluginConfig::fromJson(const QJsonObject &obj)
{
	PluginConfig cfg;
	cfg.device_name = obj.value("device_name").toString();
	cfg.listen_channel = obj.value("listen_channel").toInt(0);
	const QJsonArray arr = obj.value("mappings").toArray();
	for (const auto &v : arr) {
		const QJsonObject o = v.toObject();
		HotkeyMapping m;
		m.id = o.value("id").toString();
		if (m.id.isEmpty())
			m.id = newMappingId();
		m.name = o.value("name").toString();
		m.enabled = o.value("enabled").toBool(true);
		m.midi = matchFromJson(o.value("midi").toObject());
		m.hotkey_name = o.value("hotkey_name").toString();
		m.hotkey_label = o.value("hotkey_label").toString();
		m.hotkey_group = o.value("hotkey_group").toString();
		if (m.hotkey_group.isEmpty() && m.hotkey_label.startsWith('[')) {
			const int close = m.hotkey_label.indexOf(']');
			if (close > 1)
				m.hotkey_group = m.hotkey_label.mid(1, close - 1);
		}
		cfg.mappings.push_back(m);
	}
	return cfg;
}
