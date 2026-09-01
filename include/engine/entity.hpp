#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/inherence_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "engine/behaviour_collection.hpp"
#include "engine/registry.hpp"
#include "engine/system_participant_collection.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	struct UpdateContext;
	class Engine;

	class Entity : public ContextualizableTrait<Engine *>,
				   public Registry<Engine *, Entity>::Object,
				   public InherenceTrait<Entity>,
				   public ActivableTrait,
				   public NameTrait,
				   public SystemParticipantCollection,
				   public BehaviourCollection,
				   public EventDispatcher
	{
	private:
		using BehaviourCollection::registerBehaviour;
		using BehaviourCollection::unregisterBehaviour;
		using SystemParticipantCollection::registerParticipant;
		using SystemParticipantCollection::unregisterParticipant;
		spk::Rect2D _geometry{};

		[[nodiscard]] bool _isAcceptingInteraction() const override;
		void _propagateInteraction(
			const std::function<void(EventDispatcher *)> &callback) override;

	protected:
		virtual void _onGeometryChange(const spk::Rect2D &geometry);
		virtual void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);

	public:
		Entity(const std::string &name, Entity *parent = nullptr);

		template <typename TParticipantType, typename... TArgs>
			requires std::derived_from<TParticipantType, System::Participant>
		TParticipantType &addParticipant(TArgs &&...args)
		{
			std::unique_ptr<TParticipantType> participant =
				std::make_unique<TParticipantType>(std::forward<TArgs>(args)...);

			TParticipantType &result = *participant;
			result.attach(this);
			registerParticipant(std::move(participant));
			result.handleGeometryChange(_geometry);

			return result;
		}

		void removeParticipant(System::Participant &participant)
		{
			unregisterParticipant(participant);
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
			result.handleGeometryChange(_geometry);

			return result;
		}

		void removeBehaviour(Behaviour &behaviour)
		{
			unregisterBehaviour(behaviour);
		}

		void handleGeometryChange(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		void updateState(UpdateContext &context);
	};
}
