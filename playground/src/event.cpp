#include "event.hpp"

const char* to_string(Event event)
{
    switch (event)
    {
        case Event::NoEvent:            return "NoEvent";
        case Event::PlayerMotionIdle:   return "PlayerMotionIdle";
        case Event::PlayerMotionUp:     return "PlayerMotionUp";
        case Event::PlayerMotionLeft:   return "PlayerMotionLeft";
        case Event::PlayerMotionDown:   return "PlayerMotionDown";
        case Event::PlayerMotionRight:  return "PlayerMotionRight";
        case Event::UpdateChunkVisibility: return "UpdateChunkVisibility";
        default:                        return "UnknownEvent";
    }
}

const wchar_t* to_wstring(Event event)
{
    switch (event)
    {
        case Event::NoEvent:            return L"NoEvent";
        case Event::PlayerMotionIdle:   return L"PlayerMotionIdle";
        case Event::PlayerMotionUp:     return L"PlayerMotionUp";
        case Event::PlayerMotionLeft:   return L"PlayerMotionLeft";
        case Event::PlayerMotionDown:   return L"PlayerMotionDown";
        case Event::PlayerMotionRight:  return L"PlayerMotionRight";
        case Event::UpdateChunkVisibility: return L"UpdateChunkVisibility";
        default:                        return L"UnknownEvent";
    }
}

std::ostream& operator<<(std::ostream& os, Event event)
{
    return os << to_string(event);
}

std::wostream& operator<<(std::wostream& wos, Event event)
{
    return wos << to_wstring(event);
}