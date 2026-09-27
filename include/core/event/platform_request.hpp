#pragma once

#include <memory>
#include <variant>

#include "core/window.hpp"
#include "threading/task.hpp"

namespace spk
{
	struct NativeRegistrationRequest
	{
		Window::Identifier windowIdentifier;
		Window::Configuration configuration;
		std::shared_ptr<Window::Native> native;
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	struct NativeDeletionRequest
	{
		Window::Identifier windowIdentifier;
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	struct MousePositionRequest
	{
		Window::Identifier windowIdentifier;
		spk::Vector2Int position;
		std::shared_ptr<Task<void>> task = std::make_shared<Task<void>>();
	};

	using PlatformRequest = std::variant<NativeRegistrationRequest, NativeDeletionRequest, MousePositionRequest>;
}
