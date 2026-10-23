#pragma once

#include "engine/component.hpp"
#include "engine/publishable_attachment.hpp"

#include <string>

namespace spk
{
	class PublishableComponent : public PublishableAttachment<Component>
	{
	public:
		PublishableComponent(const std::string &name = "Unnamed publishable component", Entity *owner = nullptr);
		explicit PublishableComponent(Entity *owner);
	};
}
