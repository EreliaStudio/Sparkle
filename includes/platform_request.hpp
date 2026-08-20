#pragma once

#include <memory>
#include <variant>

#include "window.hpp"

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

	using PlatformRequest = std::variant<NativeRegistrationRequest, NativeDeletionRequest>;
}
