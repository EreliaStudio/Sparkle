#pragma once

#include <iostream>

#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/design_pattern/spk_event_notifier.hpp"

enum class Event
{
	NoEvent,
	PlayerMotionIdle,
	PlayerMotionUp,
	PlayerMotionLeft,
	PlayerMotionDown,
	PlayerMotionRight,
	UpdateChunkVisibility,
	LoadMap,
	SaveMap
};

inline const char* to_string(Event event);
inline const wchar_t* to_wstring(Event event);

std::ostream& operator<<(std::ostream& os, Event event);
std::wostream& operator<<(std::wostream& wos, Event event);

using EventCenter = spk::Singleton<spk::EventNotifier<Event>>;