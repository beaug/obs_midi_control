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

#include "mapping.hpp"

#include <QObject>
#include <QStringList>
#include <memory>

class RtMidiIn;

struct IncomingMidi {
	MidiKind kind;
	int channel; // 1-16
	int number;
	int value;
};

class MidiEngine : public QObject {
	Q_OBJECT

public:
	explicit MidiEngine(QObject *parent = nullptr);
	~MidiEngine() override;

	QStringList inputPorts() const;
	bool open(const QString &portName);
	void close();
	bool isOpen() const;
	QString currentPort() const { return current_port_; }

	void setConfig(const PluginConfig &cfg);
	PluginConfig config() const { return config_; }

	void startLearn();
	void cancelLearn();
	bool learning() const { return learning_; }

signals:
	void midiReceived(IncomingMidi msg);
	void learned(IncomingMidi msg);
	void mappingFired(QString mappingName, QString hotkeyName);
	void errorMessage(QString text);

private:
	static void rtCallback(double ts, std::vector<unsigned char> *message, void *user);
	void handleBytes(const std::vector<unsigned char> &bytes);
	void fireIfMatched(const IncomingMidi &msg);

	std::unique_ptr<RtMidiIn> midi_in_;
	QString current_port_;
	PluginConfig config_;
	bool learning_ = false;
};
