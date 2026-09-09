#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/geometry_state_trait.hpp"
#include "design_pattern/trait/inherence_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "engine/behaviour_collection.hpp"
#include "engine/registry.hpp"
#include "engine/component_collection.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	struct UpdateContext;
	class Engine;

	class Entity : public ContextualizableTrait<Engine *>,
				   public Registry<Entity, Engine *>::Object,
				   public InherenceTrait<Entity>,
				   public ActivableTrait,
				   public NameTrait,
				   public ComponentCollection,
				   public BehaviourCollection,
				   public EventDispatcher,
				   public GeometryStateTrait,
				   public UpdatableTrait,
				   public RenderSnapshotContributorTrait
	{
	private:
		using BehaviourCollection::registerBehaviour;
		using BehaviourCollection::unregisterBehaviour;
		using ComponentCollection::registerComponent;
		using ComponentCollection::unregisterComponent;
		InherenceTrait<Entity>::OnParentEditionContract _parentEditionContract;
		ContextualizableTrait<Engine *>::OnContextEditionContract _parentContextEditionContract;

		void _followParentContext(Entity *parent);
		[[nodiscard]] bool _isAcceptingEvent() const override;
		[[nodiscard]] bool _canUpdate() const override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _propagateEvent(
			const std::function<void(EventDispatcher *)> &callback) override;
		void _afterGeometryChange(const spk::Rect2D &geometry) override final;
		void _afterUpdate(UpdateContext &context) override final;
		void _afterBuildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override final;

	protected:
		void _onGeometryChange(const spk::Rect2D &geometry) override;

	public:
		Entity(const std::string &name, Entity *parent = nullptr);

		template <typename TComponentType, typename... TArgs>
			requires std::derived_from<TComponentType, Component>
		TComponentType &addComponent(TArgs &&...args)
		{
			std::unique_ptr<TComponentType> component =
				std::make_unique<TComponentType>(std::forward<TArgs>(args)...);

			TComponentType &result = *component;
			result.attach(this);
			registerComponent(std::move(component));
			result.setGeometry(geometry());

			return result;
		}

		void removeComponent(Component &component)
		{
			unregisterComponent(component);
		}

		template <typename TBehaviourType, typename... TArgs>
			requires std::derived_from<TBehaviourType, Behaviour>
		TBehaviourType &addBehaviour(TArgs &&...args)
		{
			std::unique_ptr<TBehaviourType> behaviour =
				std::make_unique<TBehaviourType>(std::forward<TArgs>(args)...);

			TBehaviourType &result = *behaviour;
			result.attach(this);
			registerBehaviour(std::move(behaviour));
			result.setGeometry(geometry());

			return result;
		}

		void removeBehaviour(Behaviour &behaviour)
		{
			unregisterBehaviour(behaviour);
		}

		[[nodiscard]] bool isEffectivelyActive() const;
	};
}
