#pragma once

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "engine/contextualizable_trait.hpp"

#include <string>

namespace spk
{
	struct UpdateContext;
	class Engine;

	class System : public ContextualizableTrait<Engine *>,
				   public ActivableTrait,
				   public NameTrait,
				   public EventDispatcher
	{
	protected:
		[[nodiscard]] bool _isAcceptingInteraction() const override;

		virtual void _updateState(UpdateContext &context);

	public:
		class Participant;
		class Participant2D;
		class Participant3D;

		System(
			const std::string &name = "Unnamed system",
			Engine *engine = nullptr);
		explicit System(Engine *engine);

		virtual ~System() = default;

		virtual void attach(Engine *engine);

		[[nodiscard]] Engine *engine();
		[[nodiscard]] const Engine *engine() const;

		void updateState(UpdateContext &context);
	};
}
