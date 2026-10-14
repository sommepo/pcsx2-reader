// SPDX-FileCopyrightText: PCSX2 Reader fork contributors (from the PPSSPP Reader fork)
// SPDX-License-Identifier: GPL-3.0+

#include <algorithm>
#include <QFontMetricsF>
#include <QPainter>
#include <cmath>
#include <QHideEvent>
#include <QShowEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPixmap>
#include <QPainterPath>
#include <QPen>
#include "reader/qt/reader_overlay.h"

namespace ReaderUi {

namespace {
constexpr int MessageMs = 3500;
} // namespace

QStringList Style::JapaneseFamilies() {
    return {QStringLiteral("Yu Gothic UI"),      QStringLiteral("Meiryo UI"),
            QStringLiteral("Meiryo"),            QStringLiteral("Noto Sans CJK JP"),
            QStringLiteral("Noto Sans JP"),      QStringLiteral("Hiragino Sans"),
            QStringLiteral("IPAexGothic"),       QStringLiteral("MS UI Gothic"),
            QStringLiteral("sans-serif")};
}

void Style::DrawOpenBook(QPainter& painter, const QRectF& area, const QColor& colour,
                         qreal width) {
    const qreal dip = area.height() * 0.22;
    QPainterPath pages;
    pages.moveTo(area.center().x(), area.top() + dip);
    pages.quadTo(area.left() + area.width() * 0.25, area.top() - dip * 0.4, area.left(),
                 area.top() + dip * 0.5);
    pages.lineTo(area.left(), area.bottom() - dip * 0.5);
    pages.quadTo(area.left() + area.width() * 0.25, area.bottom() - dip * 1.4, area.center().x(),
                 area.bottom());
    pages.quadTo(area.right() - area.width() * 0.25, area.bottom() - dip * 1.4, area.right(),
                 area.bottom() - dip * 0.5);
    pages.lineTo(area.right(), area.top() + dip * 0.5);
    pages.quadTo(area.right() - area.width() * 0.25, area.top() - dip * 0.4, area.center().x(),
                 area.top() + dip);
    pages.lineTo(area.center().x(), area.bottom());
    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(colour, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(pages);
    painter.restore();
}

// Under X11 a window manager may put a frame of its own around this window without giving
// the frame the window's "lets the pointer through": Openbox does (Batocera), and the frame
// then takes every click on the game, so no word can be clicked. With
// READER_OVERLAY_UNMANAGED=1 in the environment the window manager is kept out of it; the
// layer then lies above every window on the screen for as long as it is shown.
static Qt::WindowFlags OverlayFlags() {
    Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput |
                            Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint;
    if (qEnvironmentVariableIntValue("READER_OVERLAY_UNMANAGED") != 0) {
        flags |= Qt::X11BypassWindowManagerHint;
    }
    return flags;
}

ReaderOverlay::ReaderOverlay(QWidget* parent) : QWidget(parent, OverlayFlags()) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    message_timer.setSingleShot(true);
    connect(&message_timer, &QTimer::timeout, this, [this] {
        message.clear();
        update();
        emit ContentChanged();
    });
}

void ReaderOverlay::SetMarks(const QList<QRectF>& marks_) {
    if (marks == marks_) {
        return;
    }
    marks = marks_;
    update();
    emit ContentChanged();
}

void ReaderOverlay::SetOutlines(const QList<ReaderApp::PlacedText>& texts) {
    QList<QRectF> boxes;
    QList<QRectF> cells;
    for (const ReaderApp::PlacedText& text : texts) {
        boxes.append(text.box);
        for (const ReaderApp::PlacedText::Glyph& character : text.characters) {
            cells.append(character.drawn);
        }
    }
    if (boxes == outline_boxes && cells == outline_cells) {
        return;
    }
    outline_boxes = boxes;
    outline_cells = cells;
    update();
    emit ContentChanged();
}

void ReaderOverlay::SetSessionLine(const QString& text, const QRectF& picture, bool paused) {
    if (session_line == text && session_picture == picture && session_paused == paused) {
        return;
    }
    session_paused = paused;
    session_line = text;
    session_picture = picture;
    update();
    emit ContentChanged();
}

void ReaderOverlay::SetCursor(bool visible, const QPointF& position,
                              const std::optional<QRectF>& character) {
    if (cursor_visible == visible && cursor_position == position &&
        cursor_character == character) {
        return;
    }
    cursor_visible = visible;
    cursor_position = position;
    cursor_character = character;
    update();
    emit ContentChanged();
}

void ReaderOverlay::ShowMessage(const QString& text, const QRectF& picture) {
    message = text;
    message_picture = picture;
    message_timer.start(MessageMs);
    update();
    emit ContentChanged();
}

bool ReaderOverlay::HasContent() const {
    return !marks.isEmpty() || !outline_boxes.isEmpty() || !session_line.isEmpty() || session_paused ||
           cursor_visible || !message.isEmpty();
}

void ReaderOverlay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!outline_boxes.isEmpty()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0x4F, 0xC3, 0xF7, 0xCC), 1.5));
        for (const QRectF& box : std::as_const(outline_boxes)) {
            painter.drawRect(box);
        }
        painter.setPen(QPen(QColor(0xFF, 0x52, 0x52, 0xCC), 1.0));
        for (const QRectF& cell : std::as_const(outline_cells)) {
            painter.drawRect(cell);
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(Style::Highlight);
    for (const QRectF& mark : std::as_const(marks)) {
        painter.drawRoundedRect(mark, 3, 3);
    }

    QFont font = painter.font();
    font.setFamilies(Style::JapaneseFamilies());

    if ((!session_line.isEmpty() || session_paused) && session_picture.isValid()) {
        // Small and dim: there when looked for, not when playing.
        font.setPixelSize(std::clamp(static_cast<int>(session_picture.height() / 45.0), 11, 22));
        painter.setFont(font);
        const QFontMetricsF metrics(font);
        const qreal margin = metrics.height() * 0.5;
        const QPointF at(session_picture.right() - margin - metrics.horizontalAdvance(session_line),
                         session_picture.top() + margin + metrics.ascent());
        if (!session_line.isEmpty()) {
            painter.setPen(Style::SessionShadow);
            painter.drawText(at + QPointF(1, 1), session_line);
            painter.setPen(Style::SessionText);
            painter.drawText(at, session_line);
        }
        if (session_paused) {
            // Two bars before the line: the statistics are paused.
            const qreal height = metrics.ascent() * 0.85;
            const qreal bar = height * 0.3;
            const qreal gap = session_line.isEmpty() ? 0 : metrics.height() * 0.4;
            const qreal left = at.x() - gap - bar * 2.4;
            const qreal top = at.y() - height;
            for (const qreal x : {left, left + bar * 1.4}) {
                painter.fillRect(QRectF(x + 1, top + 1, bar, height), Style::SessionShadow);
                painter.fillRect(QRectF(x, top, bar, height), Style::Accent);
            }
        }
    }

    if (!message.isEmpty() && message_picture.isValid()) {
        font.setPixelSize(std::clamp(static_cast<int>(message_picture.height() / 36.0), 13, 26));
        painter.setFont(font);
        const QFontMetricsF metrics(font);
        const qreal pad = metrics.height() * 0.5;
        const qreal widest = message_picture.width() * 0.9 - 2 * pad;
        const QRectF text_bounds = metrics.boundingRect(QRectF(0, 0, widest, 10000),
                                                        Qt::AlignCenter | Qt::TextWordWrap, message);
        QRectF box(0, 0, text_bounds.width() + 2 * pad, text_bounds.height() + 2 * pad);
        // At the top: a game's text box is at the bottom as a rule.
        box.moveCenter(QPointF(message_picture.center().x(), 0));
        box.moveTop(message_picture.top() + metrics.height() * 2.2);
        painter.setPen(QPen(Style::Outline, 1));
        QColor background = Style::PopupBackground;
        background.setAlpha(0xF2);
        painter.setBrush(background);
        painter.drawRoundedRect(box, 6, 6);
        painter.setPen(Style::Text);
        painter.drawText(box.adjusted(pad, pad, -pad, -pad), Qt::AlignCenter | Qt::TextWordWrap,
                         message);
    }

    if (cursor_visible) {
        painter.setBrush(Qt::NoBrush);
        if (cursor_character) {
            // On a character the cursor is a frame around it.
            const QRectF frame = cursor_character->adjusted(-2, -2, 2, 2);
            painter.setPen(QPen(Style::CursorEdge, 4));
            painter.drawRoundedRect(frame, 3, 3);
            painter.setPen(QPen(Style::Accent, 2));
            painter.drawRoundedRect(frame, 3, 3);
        } else {
            Style::DrawPointer(painter, cursor_position, Style::PointerSize(this));
        }
    }
}

// ---- The console's look: the PS2's system menu: black-indigo glass, a thin light-blue edge, the headword lit from within ----

void Style::PaintPopupGround(QPainter& painter, const QRect& area) {
    QLinearGradient ground(area.topLeft(), area.bottomRight());
    ground.setColorAt(0.0, QColor(8, 12, 34));
    ground.setColorAt(1.0, QColor(2, 3, 12));
    painter.fillRect(area, ground);
    // the glass is lit a little from its upper edge
    QLinearGradient light(area.topLeft(), QPointF(area.left(), area.top() + area.height() * 0.45));
    light.setColorAt(0.0, QColor(60, 110, 255, 44));
    light.setColorAt(1.0, QColor(60, 110, 255, 0));
    painter.fillRect(area, light);
}

void Style::PaintPopupEdge(QPainter& painter, const QRect& area) {
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(PopupEdge, 1));
    painter.drawRect(QRectF(area).adjusted(0.5, 0.5, -0.5, -0.5));
}

void Style::PaintPopupRule(QPainter& painter, const QRect& rule) {
    QLinearGradient fade(rule.topLeft(), rule.topRight());
    fade.setColorAt(0.0, QColor(127, 196, 255, 180));
    fade.setColorAt(1.0, QColor(127, 196, 255, 0));
    painter.fillRect(rule, fade);
}

void Style::PaintPopupEntry(QPainter& painter, const QRectF&, const QRectF& head, bool) {
    // the headword, lit from within as the menu's chosen entry is
    const QPointF centre(head.left() + head.height() * 1.2, head.center().y());
    QRadialGradient glow(centre, head.height() * 2.4);
    glow.setColorAt(0.0, QColor(61, 125, 255, 96));
    glow.setColorAt(1.0, QColor(61, 125, 255, 0));
    painter.fillRect(head.adjusted(-8, -6, 0, 6), glow);
}

void Style::DrawPointer(QPainter& painter, const QPointF& centre, qreal size) {
    // one of the menu's lights: a white core in a blue halo
    QRadialGradient halo(centre, size / 2.0);
    halo.setColorAt(0.0, QColor(255, 255, 255, 255));
    halo.setColorAt(0.3, QColor(159, 212, 255, 220));
    halo.setColorAt(1.0, QColor(42, 107, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(halo);
    painter.drawEllipse(centre, size / 2.0, size / 2.0);
    painter.setBrush(QColor(234, 246, 255));
    painter.drawEllipse(centre, size * 0.09, size * 0.09);
}

QCursor Style::PointerCursor(qreal size, qreal ratio) {
    const int side = std::max(8, static_cast<int>(std::ceil(size * ratio)));
    QPixmap picture(side, side);
    picture.fill(Qt::transparent);
    {
        QPainter painter(&picture);
        painter.setRenderHint(QPainter::Antialiasing);
        DrawPointer(painter, QPointF(side / 2.0, side / 2.0), side);
    }
    picture.setDevicePixelRatio(ratio);
    return QCursor(picture, static_cast<int>(size / 2), static_cast<int>(size / 2));
}

qreal Style::PointerSize(const QWidget* over) {
    // seen from a sofa as well as from a desk
    return std::max(30.0, (over != nullptr ? over->height() : 720) / 20.0);
}

void ReaderOverlay::showEvent(QShowEvent* event) {
    // The game's picture is under this window, in the main window: its pointer is the console's.
    if (QWidget* host = parentWidget()) {
        host->setCursor(Style::PointerCursor(Style::PointerSize(this), devicePixelRatioF()));
    }
    QWidget::showEvent(event);
}

void ReaderOverlay::hideEvent(QHideEvent* event) {
    if (QWidget* host = parentWidget()) {
        host->unsetCursor();
    }
    QWidget::hideEvent(event);
}

} // namespace ReaderUi
