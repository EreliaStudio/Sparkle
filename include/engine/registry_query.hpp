#pragma once

#include "engine/entity.hpp"
#include "engine/registry.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TContext, typename TType>
		requires Hashable<TContext>
	class FromRegistry;

	template <typename TContext, typename TType>
		requires Hashable<TContext>
	class Registry<TContext, TType>::Query
	{
	public:
		class Operation
		{
		public:
			using OnInvalidationContractProvider = spk::ContractProvider<const TContext &>;
			using OnInvalidationCallback = typename OnInvalidationContractProvider::callback_type;
			using OnInvalidationContract = typename OnInvalidationContractProvider::Contract;

		private:
			OnInvalidationContractProvider _onInvalidationContractProvider;

		protected:
			void invalidate(const TContext &context)
			{
				_onInvalidationContractProvider.trigger(context);
			}

		public:
			virtual ~Operation() = default;

			virtual void execute(
				ElementSet &currentElements,
				const TContext &context) = 0;

			OnInvalidationContract subscribeToInvalidation(
				OnInvalidationCallback callback)
			{
				return _onInvalidationContractProvider.subscribe(callback);
			}
		};

	private:
		template <typename TOtherType>
		class RegistryOperation : public Operation
		{
		protected:
			using OtherRegistry = Registry<TContext, TOtherType>;
			using OtherElementSet = typename OtherRegistry::ElementSet;

		private:
			using OnEditionContract = typename OtherRegistry::OnEditionContract;

			struct ContractSubscription
			{
				OnEditionContract onAdditionContract;
				OnEditionContract onRemovalContract;
			};

			std::unordered_map<TContext, ContractSubscription> _subscriptions;
			std::optional<OtherElementSet> _explicitElements;

			void _ensureSubscription(const TContext &context)
			{
				if (_subscriptions.contains(context))
				{
					return;
				}

				ContractSubscription subscription;

				subscription.onAdditionContract =
					OtherRegistry::subscribeToAddition(
						context,
						[this](const TContext &context, TOtherType *) {
							this->invalidate(context);
						});

				subscription.onRemovalContract =
					OtherRegistry::subscribeToRemoval(
						context,
						[this](const TContext &context, TOtherType *) {
							this->invalidate(context);
						});

				_subscriptions.emplace(
					context,
					std::move(subscription));
			}

		protected:
			RegistryOperation() = default;

			explicit RegistryOperation(OtherElementSet elements) :
				_explicitElements(std::move(elements))
			{
			}

			[[nodiscard]] const OtherElementSet &elements(
				const TContext &context)
			{
				if (_explicitElements.has_value())
				{
					return *_explicitElements;
				}

				_ensureSubscription(context);

				return OtherRegistry::elements(context);
			}
		};

	public:
		template <typename TOtherType>
			requires requires(TType *element)
			{
				dynamic_cast<TOtherType *>(element);
			}
		class IntersectWith : public RegistryOperation<TOtherType>
		{
		private:
			using Base = RegistryOperation<TOtherType>;
			using OtherElementSet = typename Base::OtherElementSet;

		public:
			IntersectWith() = default;

			explicit IntersectWith(OtherElementSet elements) :
				Base(std::move(elements))
			{
			}

			void execute(
				ElementSet &currentElements,
				const TContext &context) override
			{
				const auto &otherElements = this->elements(context);

				for (auto it = currentElements.begin();
					 it != currentElements.end();)
				{
					TOtherType *converted =
						dynamic_cast<TOtherType *>(*it);

					if (converted == nullptr ||
						otherElements.contains(converted) == false)
					{
						it = currentElements.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		};

		template <typename TOtherType>
			requires requires(TOtherType *element)
			{
				dynamic_cast<TType *>(element);
			}
		class UnionWith : public RegistryOperation<TOtherType>
		{
		private:
			using Base = RegistryOperation<TOtherType>;
			using OtherElementSet = typename Base::OtherElementSet;

		public:
			UnionWith() = default;

			explicit UnionWith(OtherElementSet elements) :
				Base(std::move(elements))
			{
			}

			void execute(
				ElementSet &currentElements,
				const TContext &context) override
			{
				for (TOtherType *element : this->elements(context))
				{
					TType *converted =
						dynamic_cast<TType *>(element);

					if (converted != nullptr)
					{
						currentElements.insert(converted);
					}
				}
			}
		};

	private:
		struct OperationEntry
		{
			std::unique_ptr<Operation> operation;
			typename Operation::OnInvalidationContract invalidationContract;
		};

		mutable std::unordered_map<TContext, spk::CachedData<ElementSet>> _sets;
		std::vector<OperationEntry> _entries;

		ElementSet _generateSet(const TContext &context) const
		{
			ElementSet result;

			for (const auto &entry : _entries)
			{
				entry.operation->execute(result, context);
			}

			return result;
		}

		void _ensureSet(const TContext &context) const
		{
			if (_sets.contains(context))
			{
				return;
			}

			_sets.try_emplace(
				context,
				[this, context]() {
					return _generateSet(context);
				});
		}

		void _invalidate(const TContext &context) const
		{
			auto it = _sets.find(context);

			if (it != _sets.end())
			{
				it->second.invalidate();
			}
		}

	public:
		Query()
		{
			insert<FromRegistry<TContext, TType>>();
		}

		template <typename TOperationType, typename... TArgs>
		Query &insert(TArgs &&...args)
		{
			_entries.emplace_back();

			OperationEntry &entry = _entries.back();

			entry.operation =
				std::make_unique<TOperationType>(
					std::forward<TArgs>(args)...);

			entry.invalidationContract =
				entry.operation->subscribeToInvalidation(
					[this](const TContext &context) {
						_invalidate(context);
					});

			return *this;
		}

		[[nodiscard]] const ElementSet &collect(
			const TContext &context) const
		{
			_ensureSet(context);

			return _sets[context];
		}
	};

	template <typename TContext, typename TType>
		requires Hashable<TContext>
	class FromRegistry :
		public spk::Registry<TContext, TType>::Query::Operation
	{
	private:
		using RegistryType = spk::Registry<TContext, TType>;
		using ElementSet = typename RegistryType::ElementSet;
		using OnEditionContract = typename RegistryType::OnEditionContract;

		struct ContractSubscription
		{
			OnEditionContract onAdditionContract;
			OnEditionContract onRemovalContract;
		};

		std::unordered_map<TContext, ContractSubscription> _subscriptions;

		void _ensureSubscription(const TContext &context)
		{
			if (_subscriptions.contains(context))
			{
				return;
			}

			ContractSubscription subscription;

			subscription.onAdditionContract =
				RegistryType::subscribeToAddition(
					context,
					[this](const TContext &context, TType *) {
						this->invalidate(context);
					});

			subscription.onRemovalContract =
				RegistryType::subscribeToRemoval(
					context,
					[this](const TContext &context, TType *) {
						this->invalidate(context);
					});

			_subscriptions.emplace(
				context,
				std::move(subscription));
		}

	public:
		void execute(
			ElementSet &currentElements,
			const TContext &context) override
		{
			_ensureSubscription(context);

			const auto &registryElements =
				RegistryType::elements(context);

			currentElements.insert(
				registryElements.begin(),
				registryElements.end());
		}
	};

	template <typename TParticipantType>
		requires std::derived_from<TParticipantType, System::Participant>
	class ContainParticipant :
		public spk::Registry<Engine *, Entity>::Query::Operation
	{
	private:
		using Context = Engine *;
		using RegistryType = spk::Registry<Context, Entity>;
		using ElementSet = typename RegistryType::ElementSet;

		struct ContractSubscription
		{
			Entity::OnParticipantEditionContract onAdditionContract;
			Entity::OnParticipantEditionContract onRemovalContract;
		};

		using EntitySubscriptions =
			std::unordered_map<Entity *, ContractSubscription>;

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

			subscription.onAdditionContract =
				entity->subscribeToParticipantAddition(
					[this, context](System::Participant &participant) {
						if (dynamic_cast<TParticipantType *>(&participant) != nullptr)
						{
							this->invalidate(context);
						}
					});

			subscription.onRemovalContract =
				entity->subscribeToParticipantRemoval(
					[this, context](System::Participant &participant) {
						if (dynamic_cast<TParticipantType *>(&participant) != nullptr)
						{
							this->invalidate(context);
						}
					});

			subscriptions.emplace(
				entity,
				std::move(subscription));
		}

		void _updateSubscriptions(
			Context context,
			const ElementSet &elements)
		{
			auto &subscriptions = _subscriptions[context];

			for (auto it = subscriptions.begin();
				 it != subscriptions.end();)
			{
				if (elements.contains(it->first) == false)
				{
					it = subscriptions.erase(it);
				}
				else
				{
					++it;
				}
			}

			for (Entity *entity : elements)
			{
				_subscribe(context, entity);
			}
		}

	public:
		ContainParticipant() :
			_predicate([](Entity *entity) {
				return entity->getParticipant<TParticipantType>() != nullptr;
			})
		{
		}

		explicit ContainParticipant(const std::regex &regexExpression) :
			_predicate([regexExpression](Entity *entity) {
				return entity->getParticipant<TParticipantType>(
						   regexExpression) != nullptr;
			})
		{
		}

		template <typename TPredicate>
			requires std::predicate<const TPredicate &, TParticipantType *>
		explicit ContainParticipant(const TPredicate &predicate) :
			_predicate([predicate](Entity *entity) {
				return entity->getParticipant<TParticipantType>(
						   predicate) != nullptr;
			})
		{
		}

		void execute(
			ElementSet &currentElements,
			Context const &context) override
		{
			_updateSubscriptions(context, currentElements);

			for (auto it = currentElements.begin();
				 it != currentElements.end();)
			{
				if (_predicate(*it) == false)
				{
					it = currentElements.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	};


	template <typename TBehaviourType>
		requires std::derived_from<TBehaviourType, Behaviour>
	class ContainBehaviour :
		public spk::Registry<Engine *, Entity>::Query::Operation
	{
	private:
		using Context = Engine *;
		using RegistryType = spk::Registry<Context, Entity>;
		using ElementSet = typename RegistryType::ElementSet;

		struct ContractSubscription
		{
			Entity::OnBehaviourEditionContract onAdditionContract;
			Entity::OnBehaviourEditionContract onRemovalContract;
		};

		using EntitySubscriptions =
			std::unordered_map<Entity *, ContractSubscription>;

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

			subscription.onAdditionContract =
				entity->subscribeToBehaviourAddition(
					[this, context](Behaviour &behaviour) {
						if (dynamic_cast<TBehaviourType *>(&behaviour) != nullptr)
						{
							this->invalidate(context);
						}
					});

			subscription.onRemovalContract =
				entity->subscribeToBehaviourRemoval(
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
				if (elements.contains(it->first) == false)
				{
					it = subscriptions.erase(it);
				}
				else
				{
					++it;
				}
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

		void execute(ElementSet &currentElements, Context const &context) override
		{
			_updateSubscriptions(context, currentElements);

			for (auto it = currentElements.begin(); it != currentElements.end();)
			{
				if (_predicate(*it) == false)
				{
					it = currentElements.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	};

	template <typename TContext, typename TType>
		requires Hashable<TContext>
	[[nodiscard]] Registry<TContext, TType>::Query
	Registry<TContext, TType>::query()
	{
		return Query();
	}
}
