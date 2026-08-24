#pragma once

#include <memory>
#include <variant>

#include "rendering/render_snapshot.hpp"
#include "container/thread_safe_slot.hpp"
#include "core/window.hpp"

namespace spk
{
	struct SurfaceRegistrationRequest
	{
		Window::Identifier windowIdentifier;
		std::shared_ptr<Window::Surface> surface;
		spk::ThreadSafeSlot<spk::RenderSnapshot>::Consumer renderSnapshotConsumer;
		std::shared_ptr<std::atomic_bool> isRequested;
	};

	struct SurfaceCreationRequest
	{
		Window::Identifier windowIdentifier;
		std::weak_ptr<Window::Native> native;
	};

	struct SurfaceResizeRequest
	{
		Window::Identifier windowIdentifier;
		spk::Vector2UInt newSize;
	};

	struct SurfaceDeletionRequest
	{
		Window::Identifier windowIdentifier;
	};

	using RenderRequest = std::variant<
		SurfaceRegistrationRequest,
		SurfaceCreationRequest,
		SurfaceResizeRequest,
		SurfaceDeletionRequest>;
}