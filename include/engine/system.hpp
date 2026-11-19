#pragma once

#include <string>

#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "engine/contextualizable_trait.hpp"

namespace spk
{
	struct UpdateContext;
	class EngineHeadless;

	class System : public ContextualizableTrait<EngineHeadless *>,
				   public ActivableTrait,
				   public NameTrait,
				   public UpdatableTrait
	{
	protected:
		[[nodiscard]] bool _canUpdate() const override;

	public:
		System(
			const std::string &name = "Unnamed system",
			EngineHeadless *engine = nullptr);
		explicit System(EngineHeadless *engine);

		virtual ~System() = default;

		virtual void attach(EngineHeadless *engine);

		[[nodiscard]] EngineHeadless *engine();
		[[nodiscard]] const EngineHeadless *engine() const;
	};
}
