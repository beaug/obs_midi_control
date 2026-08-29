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

#include "midi-engine.hpp"

#include "obs-hotkeys.hpp"
#include "plugin-support.h"

#include "RtMidi.h"

#include <obs-module.h>

MidiEngine::MidiEngine(QObject *parent) : QObject(parent)
{
	try {
		midi_in_ = std::make_unique<RtMidiIn>(RtMidi::UNSPECIFIED, "obs-midi-hotkeys");
		midi_in_->ignoreTypes(true, true, true); // ignore sysex, time, sense
		midi_in_->setCallback(&MidiEngine::rtCallback, this);
	} catch (const RtMidiError &err) {
		obs_log(LOG_ERROR, "RtMidi init failed: %s", err.what());
	}
}

MidiEngine::~MidiEngine()
{
	close();
}

QStringList MidiEngine::inputPorts() const
{
	QStringList names;
	if (!midi_in_)
		return names;
	try {
		const unsigned n = midi_in_->getPortCount();
		for (unsigned i = 0; i < n; ++i)
			names << QString::fromStdString(midi_in_->getPortName(i));
	} catch (const RtMidiError &err) {
		obs_log(LOG_WARNING, "list ports: %s", err.what());
	}
	return names;
}

bool MidiEngine::open(const QString &portName)
{
	close();
	if (!midi_in_ || portName.isEmpty())
		return false;
	try {
		const unsigned n = midi_in_->getPortCount();
		for (unsigned i = 0; i < n; ++i) {
			if (QString::fromStdString(midi_in_->getPortName(i)) == portName) {
				midi_in_->openPort(i, "obs-midi-hotkeys");
				current_port_ = portName;
				obs_log(LOG_INFO, "opened MIDI input '%s'", portName.toUtf8().constData());
				return true;
			}
		}
	} catch (const RtMidiError &err) {
		emit errorMessage(QString::fromUtf8(err.what()));
		obs_log(LOG_ERROR, "open port failed: %s", err.what());
	}
	return false;
}

void MidiEngine::close()
{
	if (midi_in_ && midi_in_->isPortOpen()) {
		try {
			midi_in_->closePort();
		} catch (...) {
		}
	}
	current_port_.clear();
}

bool MidiEngine::isOpen() const
{
	return midi_in_ && midi_in_->isPortOpen();
}

void MidiEngine::setConfig(const PluginConfig &cfg)
{
	config_ = cfg;
}

void MidiEngine::startLearn()
{
	learning_ = true;
}

void MidiEngine::cancelLearn()
{
	learning_ = false;
}

void MidiEngine::rtCallback(double, std::vector<unsigned char> *message, void *user)
{
	if (!user || !message)
		return;
	static_cast<MidiEngine *>(user)->handleBytes(*message);
}

void MidiEngine::handleBytes(const std::vector<unsigned char> &bytes)
{
	if (bytes.size() < 2)
		return;

	const unsigned status = bytes[0];
	const unsigned type = status & 0xF0;
	const int channel = static_cast<int>((status & 0x0F) + 1);

	IncomingMidi msg{};
	msg.channel = channel;
	msg.value = 0;

	if (type == 0xB0 && bytes.size() >= 3) {
		msg.kind = MidiKind::ControlChange;
		msg.number = bytes[1] & 0x7F;
		msg.value = bytes[2] & 0x7F;
	} else if (type == 0xC0) {
		msg.kind = MidiKind::ProgramChange;
		msg.number = bytes[1] & 0x7F;
		msg.value = 0;
	} else if (type == 0x90 && bytes.size() >= 3) {
		msg.kind = MidiKind::NoteOn;
		msg.number = bytes[1] & 0x7F;
		msg.value = bytes[2] & 0x7F;
		if (msg.value == 0)
			return; // note-off encoded as note-on vel 0
	} else {
		return;
	}

	emit midiReceived(msg);

	if (learning_) {
		learning_ = false;
		emit learned(msg);
		return;
	}

	if (config_.listen_channel != 0 && config_.listen_channel != msg.channel)
		return;

	fireIfMatched(msg);
}

void MidiEngine::fireIfMatched(const IncomingMidi &msg)
{
	for (const auto &map : config_.mappings) {
		if (!map.enabled)
			continue;
		if (map.midi.kind != msg.kind)
			continue;
		if (map.midi.channel != 0 && map.midi.channel != msg.channel)
			continue;
		if (map.midi.number != msg.number)
			continue;

		bool ok = true;
		if (msg.kind != MidiKind::ProgramChange) {
			switch (map.midi.value_match) {
			case ValueMatch::Any:
				ok = true;
				break;
			case ValueMatch::Exact:
				ok = msg.value == map.midi.value_exact;
				break;
			case ValueMatch::Press:
				ok = msg.value > 0;
				break;
			}
		}
		if (!ok)
			continue;

		if (triggerObsHotkeyByName(map.hotkey_name, map.hotkey_group)) {
			emit mappingFired(map.name, map.hotkey_label.isEmpty() ? map.hotkey_name : map.hotkey_label);
			obs_log(LOG_INFO, "MIDI %s ch%d #%d -> %s [%s]",
				midiKindName(msg.kind).toUtf8().constData(), msg.channel, msg.number,
				map.hotkey_name.toUtf8().constData(), map.hotkey_group.toUtf8().constData());
		}
	}
}
