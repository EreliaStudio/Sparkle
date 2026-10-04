#pragma once

#include <string>

#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "engine/contextualizable_trait.hpp"

namespace spk
{
	struct UpdateContext;
	class Engine;

	class System : public ContextualizableTrait<Engine *>,
				   public ActivableTrait,
				   public NameTrait,
				   public UpdatableTrait
	{
	protected:
		[[nodiscard]] bool _canUpdate() const override;

	public:
		System(
			const std::string &name = "Unnamed system",
			Engine *engine = nullptr);
		explicit System(Engine *engine);

		virtual ~System() = default;

		virtual void attach(Engine *engine);

		[[nodiscard]] Engine *engine();
		[[nodiscard]] const Engine *engine() const;
	};
}
