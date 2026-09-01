#pragma once

#include "engine/entity2d.hpp"
#include "engine/system_participant.hpp"

#include <string>

namespace spk
{
	class System::Participant2D : public System::Participant,
		public spk::Registry<Engine*, System::Participant2D>::Object
	{
	public:
		Participant2D(
			const std::string &name = "Unnamed participant",
			Entity2D *owner = nullptr);
		explicit Participant2D(Entity2D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity2D *owner() override;
		[[nodiscard]] const Entity2D *owner() const override;
	};
}
