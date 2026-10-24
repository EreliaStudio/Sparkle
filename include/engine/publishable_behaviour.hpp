#pragma once

#include "engine/behaviour.hpp"
#include "engine/publishable_attachment.hpp"

#include <string>

namespace spk
{
	class PublishableBehaviour : public PublishableAttachment<Behaviour>
	{
	public:
		PublishableBehaviour(const std::string &name = "Unnamed publishable behaviour", Entity *owner = nullptr);
		explicit PublishableBehaviour(Entity *owner);
	};
}
