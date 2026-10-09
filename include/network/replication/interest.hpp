#pragma once

#include "network/message.hpp"
#include "type/uuid.hpp"

namespace spk::Network
{
	class Interest
	{
	public:
		virtual ~Interest() = default;
		[[nodiscard]] virtual spk::UUID type() const = 0;
		virtual void serialize(spk::Message::Writer &writer) const = 0;
	};
}
