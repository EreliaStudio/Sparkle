#pragma once

#include "container/query.hpp"
#include "engine/entity.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <regex>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TComponentType>
		requires std::derived_from<TComponentType, Component>
	class ContainComponent : public Query<Entity, Engine *>::Operation
	{
	private:
		using Context = Engine *;
		using ElementSet = typename Query<Entity, Context>::ElementSet;

		struct ContractSubscription
		{
			Entity::OnComponentEditionContract onAdditionContract;
			Entity::OnComponentEditionContract onRemovalContract;
			Entity::OnComponentNameEditionContract onNameEditionContract;
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
			subscription.onAdditionContract = entity->subscribeToComponentAddition(
				[this, context](Component &component) {
					if (dynamic_cast<TComponentType *>(&component) != nullptr)
					{
						this->invalidate(context);
					}
				});
			subscription.onRemovalContract = entity->subscribeToComponentRemoval(
				[this, context](Component &component) {
					if (dynamic_cast<TComponentType *>(&component) != nullptr)
					{
						this->invalidate(context);
					}
				});
			subscription.onNameEditionContract = entity->subscribeToComponentNameEdition(
				[this, context](Component &component) {
					if (dynamic_cast<TComponentType *>(&component) != nullptr)
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
		ContainComponent() :
			_predicate([](Entity *entity) {
				return entity->getComponent<TComponentType>() != nullptr;
			})
		{
		}

		explicit ContainComponent(const std::regex &regexExpression) :
			_predicate([regexExpression](Entity *entity) {
				return entity->getComponent<TComponentType>(regexExpression) != nullptr;
			})
		{
		}

		template <typename TPredicate>
			requires std::predicate<const TPredicate &, TComponentType *>
		explicit ContainComponent(const TPredicate &predicate) :
			_predicate([predicate](Entity *entity) {
				return entity->getComponent<TComponentType>(predicate) != nullptr;
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
