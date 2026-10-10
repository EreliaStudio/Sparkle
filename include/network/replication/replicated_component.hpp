#pragma once

#include "engine/component.hpp"
#include "type/uuid.hpp"

namespace spk::Network
{
	class ReplicatedComponent : public spk::Component
	{
	private:
		spk::UUID _identifier;

	public:
		explicit ReplicatedComponent(spk::UUID identifier);
		~ReplicatedComponent() override = default;

		[[nodiscard]] const spk::UUID &identifier() const noexcept;
	};
}
