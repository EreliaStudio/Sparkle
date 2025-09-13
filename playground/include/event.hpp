#pragma once

#include <sparkle.hpp>

namespace taag
{
	enum class Event
	{
		RefreshView,
		PlayerMotion
	};

	using EventDispatcher = spk::EventDispatcher<Event>;
}