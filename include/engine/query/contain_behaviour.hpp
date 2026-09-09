#pragma once

#include "engine/entity.hpp"
#include "container/query.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <regex>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TBehaviourType>
		requires std::derived_from<TBehaviourType, Behaviour>
	class ContainBehaviour : public Query<Entity, Engine *>::Operation
	{
	private:
		using Context = Engine *;
		using ElementSet = typename Query<Entity, Context>::ElementSet;

		struct ContractSubscription
		{
			Entity::OnBehaviourEditionContract onAdditionContract;
			Entity::OnBehaviourEditionContract onRemovalContract;
			Entity::OnBehaviourNameEditionContract onNameEditionContract;
		};

		using EntitySubscriptions = std::unordered_map<Entity *, ContractSubscription>;
		std::unordered_map<Context, EntitySubscriptions> _subscriptions;
		std::function<bool(Entity *)> _predicate;

		void _subscribe(Context context, Entity *entity)
		{
			auto &subscriptions = _subscriptions[context];
			if (subscriptions.contains(entity))
			{
				return;
			}

			ContractSubscription subscription;
			subscription.onAdditionContract = entity->subscribeToBehaviourAddition(
				[this, context](Behaviour &behaviour) {
					if (dynamic_cast<TBehaviourType *>(&behaviour) != nullptr)
					{
						this->invalidate(context);
					}
				});
			subscription.onRemovalContract = entity->subscribeToBehaviourRemoval(
				[this, context](Behaviour &behaviour) {
					if (dynamic_cast<TBehaviourType *>(&behaviour) != nullptr)
					{
						this->invalidate(context);
					}
				});
			subscription.onNameEditionContract = entity->subscribeToBehaviourNameEdition(
				[this, context](Behaviour &behaviour) {
					if (dynamic_cast<TBehaviourType *>(&behaviour) != nullptr)
					{
						this->invalidate(context);
					}
				});

			subscriptions.emplace(entity, std::move(subscription));
		}

		void _updateSubscriptions(Context context, const ElementSet &elements)
		{
			auto &subscriptions = _subscriptions[context];
			for (auto it = subscriptions.begin(); it != subscriptions.end();)
			{
				it = elements.contains(it->first) ? std::next(it) : subscriptions.erase(it);
			}

			for (Entity *entity : elements)
			{
				_subscribe(context, entity);
			}
		}

	public:
		ContainBehaviour() :
			_predicate([](Entity *entity) {
				return entity->getBehaviour<TBehaviourType>() != nullptr;
			})
		{
		}

		explicit ContainBehaviour(const std::regex &regexExpression) :
			_predicate([regexExpression](Entity *entity) {
				return entity->getBehaviour<TBehaviourType>(regexExpression) != nullptr;
			})
		{
		}

		template <typename TPredicate>
			requires std::predicate<const TPredicate &, TBehaviourType *>
		explicit ContainBehaviour(const TPredicate &predicate) :
			_predicate([predicate](Entity *entity) {
				return entity->getBehaviour<TBehaviourType>(predicate) != nullptr;
			})
		{
		}

		void execute(ElementSet &currentElements, const Context &context) override
		{
			_updateSubscriptions(context, currentElements);
			for (auto it = currentElements.begin(); it != currentElements.end();)
			{
				it = _predicate(*it) ? std::next(it) : currentElements.erase(it);
			}
		}
	};
}
