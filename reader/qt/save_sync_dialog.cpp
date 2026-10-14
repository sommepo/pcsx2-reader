// SPDX-FileCopyrightText: PCSX2 Reader fork contributors
// SPDX-License-Identifier: GPL-3.0+

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include "reader/core/emulator.h"
#include "reader/qt/save_sync_dialog.h"
#include "reader/save_sync/host.h"

namespace ReaderUi {

SaveSyncDialog::SaveSyncDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Save sync"));
    setMinimumWidth(560);
    Reader::Emu::EnsureSaveSync();
    const SaveSync::Host::Settings now = SaveSync::Host::Get();

    auto* layout = new QVBoxLayout(this);
    auto* about = new QLabel(
        tr("Every game has a memory card of its own, and that card follows you to your other devices: "
           "the newest save is fetched before a game starts and sent after it saves. The service is the "
           "one the other reader apps use."),
        this);
    about->setWordWrap(true);
    layout->addWidget(about);

    enabled = new QCheckBox(tr("Sync saves with my other devices"), this);
    enabled->setChecked(now.enabled);
    layout->addWidget(enabled);

    auto* form = new QFormLayout;
    addresses = new QLineEdit(QString::fromStdString(now.addresses), this);
    addresses->setPlaceholderText(tr("address, or address:port; several with commas"));
    code = new QLineEdit(QString::fromStdString(now.code), this);
    code->setPlaceholderText(tr("the code the service shows"));
    device = new QLineEdit(QString::fromStdString(now.device), this);
    form->addRow(tr("Service"), addresses);
    form->addRow(tr("Code"), code);
    form->addRow(tr("This device's name"), device);
    layout->addLayout(form);

    auto* buttons = new QHBoxLayout;
    auto* check = new QPushButton(tr("Check the service"), this);
    auto* all = new QPushButton(tr("Sync every game now"), this);
    buttons->addWidget(check);
    buttons->addWidget(all);
    buttons->addStretch();
    layout->addLayout(buttons);

    status = new QLabel(this);
    status->setWordWrap(true);
    layout->addWidget(status);
    conflicts = new QVBoxLayout;
    layout->addLayout(conflicts);
    layout->addStretch();

    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(enabled, &QCheckBox::toggled, this, [this] { Store(); });
    for (QLineEdit* field : {addresses, code, device}) {
        connect(field, &QLineEdit::editingFinished, this, [this] { Store(); });
    }
    connect(check, &QPushButton::clicked, this, [this] {
        Store();
        SaveSync::Host::Check();
    });
    connect(all, &QPushButton::clicked, this, [this] {
        Store();
        SaveSync::Host::SyncAll();
    });
    timer = new QTimer(this);
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, [this] { Refresh(); });
    timer->start();
    Refresh();
}

void SaveSyncDialog::Store() {
    SaveSync::Host::Settings settings;
    settings.enabled = enabled->isChecked();
    settings.addresses = addresses->text().trimmed().toStdString();
    settings.code = code->text().trimmed().toStdString();
    settings.device = device->text().trimmed().toStdString();
    const SaveSync::Host::Settings before = SaveSync::Host::Get();
    if (before.enabled != settings.enabled || before.addresses != settings.addresses || before.code != settings.code ||
        before.device != settings.device) {
        SaveSync::Host::Set(settings);
    }
}

void SaveSyncDialog::Refresh() {
    const std::uint64_t generation = SaveSync::Host::Generation();
    if (generation == seen) {
        return;
    }
    seen = generation;
    status->setText(QString::fromStdString(SaveSync::Host::Status()));
    while (QLayoutItem* item = conflicts->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    for (const SaveSync::Host::Conflict& conflict : SaveSync::Host::Conflicts()) {
        auto* row = new QWidget(this);
        auto* line = new QHBoxLayout(row);
        line->setContentsMargins(0, 0, 0, 0);
        const QString name = QString::fromStdString(conflict.name.empty() ? conflict.disc : conflict.name);
        auto* text = new QLabel(QStringLiteral("<b>%1</b><br>%2").arg(name.toHtmlEscaped(), QString::fromStdString(conflict.message).toHtmlEscaped()), row);
        text->setWordWrap(true);
        line->addWidget(text, 1);
        auto* mine = new QPushButton(tr("Keep this device's"), row);
        auto* theirs = new QPushButton(tr("Take the other's"), row);
        line->addWidget(mine);
        line->addWidget(theirs);
        const std::string title = conflict.title;
        connect(mine, &QPushButton::clicked, this, [title] { SaveSync::Host::Resolve(title, true); });
        connect(theirs, &QPushButton::clicked, this, [title] { SaveSync::Host::Resolve(title, false); });
        conflicts->addWidget(row);
    }
}

} // namespace ReaderUi
