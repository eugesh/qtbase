// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmevent.h"

QT_BEGIN_NAMESPACE

namespace KeyboardModifier
{
template <>
QFlags<Qt::KeyboardModifier> getForEvent<EmscriptenKeyboardEvent>(
    const EmscriptenKeyboardEvent& event)
{
    return internal::Helper<EmscriptenKeyboardEvent>::getModifierForEvent(event) |
        (event.location == DOM_KEY_LOCATION_NUMPAD ? Qt::KeypadModifier : Qt::NoModifier);
}
}  // namespace KeyboardModifier

Event::Event(EventType type, emscripten::val target) : type(type), target(target) { }

Event::~Event() = default;

Event::Event(const Event &other) = default;

Event::Event(Event &&other) = default;

Event &Event::operator=(const Event &other) = default;

Event &Event::operator=(Event &&other) = default;

MouseEvent::MouseEvent(EventType type, emscripten::val event) : Event(type, event["target"])
{
    mouseButton = MouseEvent::buttonFromWeb(event["button"].as<int>());
    mouseButtons = MouseEvent::buttonsFromWeb(event["buttons"].as<unsigned short>());
    // The current button state (event.buttons) may be out of sync for some PointerDown
    // events where the "down" state is very brief, for example taps on Apple trackpads.
    // Qt expects that the current button state is in sync with the event, so we sync
    // it up here.
    if (type == EventType::PointerDown)
        mouseButtons |= mouseButton;
    localPoint = QPoint(event["offsetX"].as<int>(), event["offsetY"].as<int>());
    pointInPage = QPoint(event["pageX"].as<int>(), event["pageY"].as<int>());
    pointInViewport = QPoint(event["clientX"].as<int>(), event["clientY"].as<int>());
    modifiers = KeyboardModifier::getForEvent(event);
}

MouseEvent::~MouseEvent() = default;

MouseEvent::MouseEvent(const MouseEvent &other) = default;

MouseEvent::MouseEvent(MouseEvent &&other) = default;

MouseEvent &MouseEvent::operator=(const MouseEvent &other) = default;

MouseEvent &MouseEvent::operator=(MouseEvent &&other) = default;

PointerEvent::PointerEvent(EventType type, emscripten::val event) : MouseEvent(type, event)
{
    pointerId = event["pointerId"].as<int>();
    pointerType = event["pointerType"].as<std::string>() == "mouse" ? PointerType::Mouse
                                                                    : PointerType::Other;
}

PointerEvent::~PointerEvent() = default;

PointerEvent::PointerEvent(const PointerEvent &other) = default;

PointerEvent::PointerEvent(PointerEvent &&other) = default;

PointerEvent &PointerEvent::operator=(const PointerEvent &other) = default;

PointerEvent &PointerEvent::operator=(PointerEvent &&other) = default;

std::optional<PointerEvent> PointerEvent::fromWeb(emscripten::val event)
{
    const auto eventType = ([&event]() -> std::optional<EventType> {
        const auto eventTypeString = event["type"].as<std::string>();

        if (eventTypeString == "pointermove")
            return EventType::PointerMove;
        else if (eventTypeString == "pointerup")
            return EventType::PointerUp;
        else if (eventTypeString == "pointerdown")
            return EventType::PointerDown;
        else if (eventTypeString == "pointerenter")
            return EventType::PointerEnter;
        else if (eventTypeString == "pointerleave")
            return EventType::PointerLeave;
        return std::nullopt;
    })();
    if (!eventType)
        return std::nullopt;

    return PointerEvent(*eventType, event);
}

DragEvent::DragEvent(EventType type, emscripten::val event)
    : MouseEvent(type, event), dataTransfer(event["dataTransfer"])
{
    dropAction = ([event]() {
        const std::string effect = event["dataTransfer"]["dropEffect"].as<std::string>();

        if (effect == "copy")
            return Qt::CopyAction;
        else if (effect == "move")
            return Qt::MoveAction;
        else if (effect == "link")
            return Qt::LinkAction;
        return Qt::IgnoreAction;
    })();
}

DragEvent::~DragEvent() = default;

DragEvent::DragEvent(const DragEvent &other) = default;

DragEvent::DragEvent(DragEvent &&other) = default;

DragEvent &DragEvent::operator=(const DragEvent &other) = default;

DragEvent &DragEvent::operator=(DragEvent &&other) = default;

std::optional<DragEvent> DragEvent::fromWeb(emscripten::val event)
{
    const auto eventType = ([&event]() -> std::optional<EventType> {
        const auto eventTypeString = event["type"].as<std::string>();

        if (eventTypeString == "drop")
            return EventType::Drop;
        return std::nullopt;
    })();
    if (!eventType)
        return std::nullopt;
    return DragEvent(*eventType, event);
}

WheelEvent::WheelEvent(EventType type, emscripten::val event) : MouseEvent(type, event)
{
    deltaMode = ([event]() {
        const int deltaMode = event["deltaMode"].as<int>();
        const auto jsWheelEventType = emscripten::val::global("WheelEvent");
        if (deltaMode == jsWheelEventType["DOM_DELTA_PIXEL"].as<int>())
            return DeltaMode::Pixel;
        else if (deltaMode == jsWheelEventType["DOM_DELTA_LINE"].as<int>())
            return DeltaMode::Line;
        return DeltaMode::Page;
    })();

    delta = QPoint(event["deltaX"].as<int>(), event["deltaY"].as<int>());

    webkitDirectionInvertedFromDevice = event["webkitDirectionInvertedFromDevice"].as<bool>();
}

WheelEvent::~WheelEvent() = default;

WheelEvent::WheelEvent(const WheelEvent &other) = default;

WheelEvent::WheelEvent(WheelEvent &&other) = default;

WheelEvent &WheelEvent::operator=(const WheelEvent &other) = default;

WheelEvent &WheelEvent::operator=(WheelEvent &&other) = default;

std::optional<WheelEvent> WheelEvent::fromWeb(emscripten::val event)
{
    const auto eventType = ([&event]() -> std::optional<EventType> {
        const auto eventTypeString = event["type"].as<std::string>();

        if (eventTypeString == "wheel")
            return EventType::Wheel;
        return std::nullopt;
    })();
    if (!eventType)
        return std::nullopt;
    return WheelEvent(*eventType, event);
}

QT_END_NAMESPACE
