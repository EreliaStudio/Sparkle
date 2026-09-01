#pragma once

#include "engine/contextualizable_trait.hpp"
#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"

#include <string>

namespace spk
{
	class Engine;
	class Entity;

	class EntityAttachment : public ContextualizableTrait<Engine *>,
							 public ActivableTrait,
							 public NameTrait
	{
	private:
		using OwnerContextualizableTrait = ContextualizableTrait<Engine *>;

		Entity *_owner = nullptr;
		OwnerContextualizableTrait::OnContextEditionContract _onOwnerContextEditionContract;

	public:
		EntityAttachment(
			const std::string &name = "Unnamed entity attachment",
			Entity *owner = nullptr);
		explicit EntityAttachment(Entity *owner);

		virtual ~EntityAttachment() = default;

		virtual void attach(Entity *owner);

		[[nodiscard]] virtual Entity *owner();
		[[nodiscard]] virtual const Entity *owner() const;
	};
}
