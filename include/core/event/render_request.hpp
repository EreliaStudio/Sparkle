#pragma once

#include <memory>
#include <variant>

#include "container/thread_safe_slot.hpp"
#include "core/window.hpp"
#include "threading/task.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	struct SurfaceRegistrationRequest
	{
		Window::Identifier windowIdentifier;
		std::shared_ptr<Window::Surface> surface;
		spk::ThreadSafeSlot<spk::RenderSnapshot>::Consumer renderSnapshotConsumer;
		std::shared_ptr<std::atomic_bool> isRequested;
			std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	struct SurfaceCreationRequest
	{
		Window::Identifier windowIdentifier;
		std::weak_ptr<Window::Native> native;
			std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	struct SurfaceResizeRequest
	{
		Window::Identifier windowIdentifier;
		spk::Vector2UInt newSize;
			std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	struct SurfaceDeletionRequest
	{
		Window::Identifier windowIdentifier;
	
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
		std::shared_ptr<Task<void>> nativeDeletionTask = std::make_shared<Task<void>>();
	};

	using RenderRequest = std::variant<
		SurfaceRegistrationRequest,
		SurfaceCreationRequest,
		SurfaceResizeRequest,
		SurfaceDeletionRequest>;
}