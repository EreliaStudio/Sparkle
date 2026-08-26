#pragma once

#include <memory>
#include <variant>

#include "container/thread_safe_slot.hpp"
#include "core/window.hpp"
#include "graphics/color.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	struct StateRegistrationRequest
	{
		Window::Identifier windowIdentifier;
		spk::Color backgroundColor;
		std::shared_ptr<Window::State> state;
		spk::ThreadSafeSlot<spk::RenderSnapshot>::Producer renderSnapshotProducer;
		std::shared_ptr<std::atomic_bool> isRequested;
	};

	struct StateDeletionRequest
	{
		Window::Identifier windowIdentifier;
	};

	using UpdateRequest = std::variant<
		StateRegistrationRequest,
		StateDeletionRequest>;
}