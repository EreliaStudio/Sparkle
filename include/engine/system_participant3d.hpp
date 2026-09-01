#pragma once

#include "engine/entity3d.hpp"
#include "engine/system_participant.hpp"

#include <string>

namespace spk
{
	class System::Participant3D : public System::Participant,
		public spk::Registry<Engine*, System::Participant3D>::Object
	{
	public:
		Participant3D(
			const std::string &name = "Unnamed participant",
			Entity3D *owner = nullptr);
		explicit Participant3D(Entity3D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity3D *owner() override;
		[[nodiscard]] const Entity3D *owner() const override;
	};
}
