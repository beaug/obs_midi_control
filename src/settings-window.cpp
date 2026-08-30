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

#include "settings-window.hpp"

#include "obs-hotkeys.hpp"

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(MidiEngine *engine, QWidget *parent)
	: QDialog(parent),
	  engine_(engine)
{
	setWindowTitle(QStringLiteral("MIDI Hotkeys"));
	resize(980, 520);

	auto *root = new QVBoxLayout(this);

	auto *devRow = new QHBoxLayout();
	devRow->addWidget(new QLabel(QStringLiteral("MIDI device")));
	device_box_ = new QComboBox;
	device_box_->setMinimumWidth(240);
	devRow->addWidget(device_box_, 1);

	auto *refresh = new QPushButton(QStringLiteral("Refresh"));
	connect(refresh, &QPushButton::clicked, this, &SettingsWindow::refreshDevices);
	devRow->addWidget(refresh);

	auto *connectBtn = new QPushButton(QStringLiteral("Connect"));
	connect(connectBtn, &QPushButton::clicked, this, &SettingsWindow::connectDevice);
	devRow->addWidget(connectBtn);

	devRow->addWidget(new QLabel(QStringLiteral("Listen channel")));
	listen_ch_box_ = new QComboBox;
	listen_ch_box_->addItem(QStringLiteral("Any"), 0);
	for (int ch = 1; ch <= 16; ++ch)
		listen_ch_box_->addItem(QString::number(ch), ch);
	connect(listen_ch_box_, &QComboBox::currentIndexChanged, this, &SettingsWindow::persist);
	devRow->addWidget(listen_ch_box_);
	root->addLayout(devRow);

	status_ = new QLabel(QStringLiteral("Not connected"));
	root->addWidget(status_);
	monitor_ = new QLabel(QStringLiteral("Last MIDI: —"));
	root->addWidget(monitor_);

	table_ = new QTableWidget(0, 8);
	table_->setHorizontalHeaderLabels({QStringLiteral("On"), QStringLiteral("Name"), QStringLiteral("Ch"),
					   QStringLiteral("Type"), QStringLiteral("CC/PC/Note"),
					   QStringLiteral("Value"), QStringLiteral("Hotkey"),
					   QStringLiteral("Hotkey id")});
	table_->horizontalHeader()->setStretchLastSection(true);
	table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);
	table_->verticalHeader()->setVisible(false);
	root->addWidget(table_, 1);

	auto *btnRow = new QHBoxLayout();
	auto *addBtn = new QPushButton(QStringLiteral("Add mapping"));
	connect(addBtn, &QPushButton::clicked, this, &SettingsWindow::addMapping);
	btnRow->addWidget(addBtn);

	auto *rmBtn = new QPushButton(QStringLiteral("Remove"));
	connect(rmBtn, &QPushButton::clicked, this, &SettingsWindow::removeSelected);
	btnRow->addWidget(rmBtn);

	learn_btn_ = new QPushButton(QStringLiteral("Learn selected"));
	connect(learn_btn_, &QPushButton::clicked, this, &SettingsWindow::learnSelected);
	btnRow->addWidget(learn_btn_);

	btnRow->addStretch(1);

	auto *saveBtn = new QPushButton(QStringLiteral("Save"));
	connect(saveBtn, &QPushButton::clicked, this, &SettingsWindow::persist);
	btnRow->addWidget(saveBtn);
	root->addLayout(btnRow);

	connect(engine_, &MidiEngine::learned, this, &SettingsWindow::onLearned);
	connect(engine_, &MidiEngine::midiReceived, this, &SettingsWindow::onMidiMonitor);
	connect(engine_, &MidiEngine::errorMessage, this, [this](const QString &t) {
		status_->setText(t);
	});

	refreshDevices();
}

void SettingsWindow::loadFrom(const PluginConfig &cfg)
{
	const QSignalBlocker blockListen(listen_ch_box_);
	const QSignalBlocker blockDevice(device_box_);

	cfg_ = cfg;

	const int devIdx = device_box_->findText(cfg_.device_name);
	if (devIdx >= 0)
		device_box_->setCurrentIndex(devIdx);
	else if (!cfg_.device_name.isEmpty()) {
		device_box_->addItem(cfg_.device_name);
		device_box_->setCurrentText(cfg_.device_name);
	}

	const int chIdx = listen_ch_box_->findData(cfg_.listen_channel);
	if (chIdx >= 0)
		listen_ch_box_->setCurrentIndex(chIdx);

	rebuildTable();

	if (!cfg_.device_name.isEmpty()) {
		if (engine_->open(cfg_.device_name))
			status_->setText(QStringLiteral("Connected: ") + cfg_.device_name);
	}
}

PluginConfig SettingsWindow::currentConfig() const
{
	PluginConfig out = cfg_;
	out.device_name = device_box_->currentText();
	out.listen_channel = listen_ch_box_->currentData().toInt();
	out.mappings.clear();
	for (int row = 0; row < table_->rowCount(); ++row) {
		HotkeyMapping m;
		if (row < cfg_.mappings.size())
			m.id = cfg_.mappings[row].id;
		if (m.id.isEmpty())
			m.id = newMappingId();
		applyRowToMapping(row, m);
		out.mappings.push_back(m);
	}
	return out;
}

void SettingsWindow::refreshDevices()
{
	const QString current = device_box_->currentText();
	device_box_->clear();
	const QStringList ports = engine_->inputPorts();
	device_box_->addItems(ports);
	const int idx = device_box_->findText(current);
	if (idx >= 0)
		device_box_->setCurrentIndex(idx);
}

void SettingsWindow::connectDevice()
{
	const QString name = device_box_->currentText();
	if (engine_->open(name)) {
		status_->setText(QStringLiteral("Connected: ") + name);
		persist();
	} else {
		status_->setText(QStringLiteral("Failed to open ") + name);
	}
}

void SettingsWindow::addMapping()
{
	HotkeyMapping m;
	m.id = newMappingId();
	m.name = QStringLiteral("Mapping %1").arg(cfg_.mappings.size() + 1);
	m.midi.channel = cfg_.listen_channel;
	m.midi.kind = MidiKind::ControlChange;
	m.midi.number = 1;
	m.midi.value_match = ValueMatch::Press;
	cfg_.mappings.push_back(m);
	rebuildTable();
	table_->selectRow(table_->rowCount() - 1);
}

void SettingsWindow::removeSelected()
{
	const int row = selectedRow();
	if (row < 0)
		return;
	cfg_ = currentConfig();
	if (row >= 0 && row < cfg_.mappings.size())
		cfg_.mappings.removeAt(row);
	rebuildTable();
	persist();
}

void SettingsWindow::learnSelected()
{
	if (selectedRow() < 0) {
		QMessageBox::information(this, QStringLiteral("Learn"),
					 QStringLiteral("Select a mapping row first, then press a control on the MIDI device."));
		return;
	}
	if (!engine_->isOpen()) {
		QMessageBox::warning(this, QStringLiteral("Learn"),
				     QStringLiteral("Connect a MIDI device first."));
		return;
	}
	engine_->startLearn();
	learn_btn_->setText(QStringLiteral("Waiting for MIDI…"));
	status_->setText(QStringLiteral("Learn: move a knob or press a pad / program change"));
}

void SettingsWindow::onLearned(IncomingMidi msg)
{
	learn_btn_->setText(QStringLiteral("Learn selected"));
	const int row = selectedRow();
	if (row < 0)
		return;

	auto *ch = qobject_cast<QSpinBox *>(table_->cellWidget(row, 2));
	auto *kind = qobject_cast<QComboBox *>(table_->cellWidget(row, 3));
	auto *num = qobject_cast<QSpinBox *>(table_->cellWidget(row, 4));
	auto *val = qobject_cast<QComboBox *>(table_->cellWidget(row, 5));
	if (!ch || !kind || !num || !val)
		return;

	ch->setValue(msg.channel);
	kind->setCurrentIndex(static_cast<int>(msg.kind));
	num->setValue(msg.number);
	if (msg.kind == MidiKind::ProgramChange)
		val->setCurrentIndex(static_cast<int>(ValueMatch::Any));
	else if (msg.value == 0 || msg.value == 127)
		val->setCurrentIndex(static_cast<int>(ValueMatch::Press));
	else
		val->setCurrentIndex(static_cast<int>(ValueMatch::Exact));

	status_->setText(QStringLiteral("Learned %1 ch%2 #%3 value %4")
				 .arg(midiKindName(msg.kind))
				 .arg(msg.channel)
				 .arg(msg.number)
				 .arg(msg.value));
	persist();
}

void SettingsWindow::onMidiMonitor(IncomingMidi msg)
{
	monitor_->setText(QStringLiteral("Last MIDI: %1  ch %2  #%3  value %4")
				  .arg(midiKindName(msg.kind))
				  .arg(msg.channel)
				  .arg(msg.number)
				  .arg(msg.value));
}

void SettingsWindow::persist()
{
	cfg_ = currentConfig();
	engine_->setConfig(cfg_);
	emit configChanged(cfg_);
}

void SettingsWindow::fillHotkeyCombo(QComboBox *box, const QString &selectedName, const QString &selectedGroup)
{
	box->clear();
	box->addItem(QStringLiteral("(choose hotkey)"), QVariant::fromValue(QStringList{}));
	const auto keys = enumerateObsHotkeys();
	int select = 0;
	int i = 1;
	for (const auto &k : keys) {
		const QString label = QStringLiteral("[%1] %2").arg(k.group, k.description);
		box->addItem(label, QVariant::fromValue(QStringList{k.name, k.group}));
		if (k.name == selectedName && (selectedGroup.isEmpty() || k.group == selectedGroup))
			select = i;
		++i;
	}
	box->setCurrentIndex(select);
}

void SettingsWindow::rebuildTable()
{
	table_->blockSignals(true);
	table_->setRowCount(0);
	for (const auto &m : cfg_.mappings) {
		const int row = table_->rowCount();
		table_->insertRow(row);

		auto *on = new QComboBox;
		on->addItem(QStringLiteral("Off"), false);
		on->addItem(QStringLiteral("On"), true);
		on->setCurrentIndex(m.enabled ? 1 : 0);
		connect(on, &QComboBox::currentIndexChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 0, on);

		auto *name = new QLineEdit(m.name);
		connect(name, &QLineEdit::editingFinished, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 1, name);

		auto *ch = new QSpinBox;
		ch->setRange(0, 16);
		ch->setSpecialValueText(QStringLiteral("Any"));
		ch->setValue(m.midi.channel);
		connect(ch, &QSpinBox::valueChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 2, ch);

		auto *kind = new QComboBox;
		kind->addItem(QStringLiteral("CC"), static_cast<int>(MidiKind::ControlChange));
		kind->addItem(QStringLiteral("PC"), static_cast<int>(MidiKind::ProgramChange));
		kind->addItem(QStringLiteral("Note"), static_cast<int>(MidiKind::NoteOn));
		kind->setCurrentIndex(static_cast<int>(m.midi.kind));
		connect(kind, &QComboBox::currentIndexChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 3, kind);

		auto *num = new QSpinBox;
		num->setRange(0, 127);
		num->setValue(m.midi.number);
		connect(num, &QSpinBox::valueChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 4, num);

		auto *val = new QComboBox;
		val->addItem(QStringLiteral("Any value"), static_cast<int>(ValueMatch::Any));
		val->addItem(QStringLiteral("Exact 127"), static_cast<int>(ValueMatch::Exact));
		val->addItem(QStringLiteral("Press (>0)"), static_cast<int>(ValueMatch::Press));
		val->setCurrentIndex(static_cast<int>(m.midi.value_match));
		connect(val, &QComboBox::currentIndexChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 5, val);

		auto *hk = new QComboBox;
		hk->setEditable(false);
		fillHotkeyCombo(hk, m.hotkey_name, m.hotkey_group);
		connect(hk, &QComboBox::currentIndexChanged, this, &SettingsWindow::persist);
		table_->setCellWidget(row, 6, hk);

		auto *idLabel = new QLabel(m.hotkey_name);
		idLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
		table_->setCellWidget(row, 7, idLabel);
	}
	table_->blockSignals(false);
}

void SettingsWindow::applyRowToMapping(int row, HotkeyMapping &m) const
{
	auto *on = qobject_cast<QComboBox *>(table_->cellWidget(row, 0));
	auto *name = qobject_cast<QLineEdit *>(table_->cellWidget(row, 1));
	auto *ch = qobject_cast<QSpinBox *>(table_->cellWidget(row, 2));
	auto *kind = qobject_cast<QComboBox *>(table_->cellWidget(row, 3));
	auto *num = qobject_cast<QSpinBox *>(table_->cellWidget(row, 4));
	auto *val = qobject_cast<QComboBox *>(table_->cellWidget(row, 5));
	auto *hk = qobject_cast<QComboBox *>(table_->cellWidget(row, 6));
	if (!on || !name || !ch || !kind || !num || !val || !hk)
		return;

	m.enabled = on->currentData().toBool();
	m.name = name->text();
	m.midi.channel = ch->value();
	m.midi.kind = static_cast<MidiKind>(kind->currentData().toInt());
	m.midi.number = num->value();
	m.midi.value_match = static_cast<ValueMatch>(val->currentData().toInt());
	m.midi.value_exact = 127;
	const QStringList parts = hk->currentData().toStringList();
	m.hotkey_name = parts.value(0);
	m.hotkey_group = parts.value(1);
	m.hotkey_label = hk->currentText();
	if (m.hotkey_group.isEmpty() && m.hotkey_label.startsWith('[')) {
		const int close = m.hotkey_label.indexOf(']');
		if (close > 1)
			m.hotkey_group = m.hotkey_label.mid(1, close - 1);
	}
}

int SettingsWindow::selectedRow() const
{
	const auto rows = table_->selectionModel()->selectedRows();
	return rows.isEmpty() ? -1 : rows.first().row();
}
