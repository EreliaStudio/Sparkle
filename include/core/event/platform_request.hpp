#pragma once

#include <memory>
#include <variant>

#include "core/window.hpp"

namespace spk
{
	struct NativeRegistrationRequest
	{
		Window::Identifier windowIdentifier;
		Window::Configuration configuration;
		std::shared_ptr<Window::Native> native;
	};

	struct NativeDeletionRequest
	{
		Window::Identifier windowIdentifier;
	};

	struct MousePositionRequest
	{
		Window::Identifier windowIdentifier;
		spk::Vector2Int position;
	};

	using PlatformRequest = std::variant<NativeRegistrationRequest, NativeDeletionRequest, MousePositionRequest>;
}
