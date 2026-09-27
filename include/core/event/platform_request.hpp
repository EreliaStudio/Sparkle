#pragma once

#include <memory>
#include <variant>

#include "core/window.hpp"
#include "threading/task.hpp"

namespace spk
{
	struct NativeRegistrationRequest
	{
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
		Window::Identifier windowIdentifier;
		Window::Configuration configuration;
		std::shared_ptr<Window::Native> native;
	};

	struct NativeDeletionRequest
	{
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
		Window::Identifier windowIdentifier;
	};

	struct MousePositionRequest
	{
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
		Window::Identifier windowIdentifier;
		spk::Vector2Int position;
	};

	using PlatformRequest = std::variant<NativeRegistrationRequest, NativeDeletionRequest, MousePositionRequest>;
}
