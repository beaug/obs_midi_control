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
#include "midi-engine.hpp"

#include <QDialog>

class QComboBox;
class QSpinBox;
class QTableWidget;
class QLabel;
class QPushButton;

class SettingsWindow : public QDialog {
	Q_OBJECT

public:
	explicit SettingsWindow(MidiEngine *engine, QWidget *parent = nullptr);

	void loadFrom(const PluginConfig &cfg);
	PluginConfig currentConfig() const;

signals:
	void configChanged(PluginConfig cfg);

private slots:
	void refreshDevices();
	void connectDevice();
	void addMapping();
	void removeSelected();
	void learnSelected();
	void onLearned(IncomingMidi msg);
	void onMidiMonitor(IncomingMidi msg);
	void persist();

private:
	void rebuildTable();
	void applyRowToMapping(int row, HotkeyMapping &m) const;
	int selectedRow() const;
	void fillHotkeyCombo(QComboBox *box, const QString &selectedName, const QString &selectedGroup = QString());

	MidiEngine *engine_;
	PluginConfig cfg_;

	QComboBox *device_box_ = nullptr;
	QComboBox *listen_ch_box_ = nullptr;
	QLabel *status_ = nullptr;
	QLabel *monitor_ = nullptr;
	QTableWidget *table_ = nullptr;
	QPushButton *learn_btn_ = nullptr;
};
