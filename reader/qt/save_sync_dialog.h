// SPDX-FileCopyrightText: PCSX2 Reader fork contributors
// SPDX-License-Identifier: GPL-3.0+

#pragma once

// Save sync's page: the service's address and code, this device's name, the switch; what
// the service answers; and two devices' different saves, for the user to choose between.

#include <cstdint>
#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QTimer;
class QVBoxLayout;

namespace ReaderUi {

class SaveSyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveSyncDialog(QWidget* parent = nullptr);

private:
    void Store();
    void Refresh();

    QCheckBox* enabled{};
    QLineEdit* addresses{};
    QLineEdit* code{};
    QLineEdit* device{};
    QLabel* status{};
    QVBoxLayout* conflicts{};
    QTimer* timer{};
    std::uint64_t seen{~0ull};
};

} // namespace ReaderUi
