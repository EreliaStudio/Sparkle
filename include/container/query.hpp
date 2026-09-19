#pragma once

#include "concept.hpp"
#include "container/cached_data.hpp"
#include "design_pattern/trait/query_source_trait.hpp"

#include <concepts>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class Query : public QuerySourceTrait<TType, TContext>
	{
	public:
		using Base = QuerySourceTrait<TType, TContext>;
		using ElementSet = typename Base::ElementSet;

		class Operation
		{
		public:
			using OnInvalidationContractProvider = ContractProvider<const TContext &>;
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

			virtual void execute(ElementSet &currentElements, const TContext &context) = 0;

			[[nodiscard]] OnInvalidationContract subscribeToInvalidation(OnInvalidationCallback callback)
			{
				return _onInvalidationContractProvider.subscribe(std::move(callback));
			}
		};

	private:
		struct OperationEntry
		{
			std::unique_ptr<Operation> operation;
			typename Operation::OnInvalidationContract invalidationContract;
		};

		mutable std::unordered_map<TContext, CachedData<ElementSet>> _sets;
		std::vector<OperationEntry> _operations;

		[[nodiscard]] ElementSet _generateSet(const TContext &context) const
		{
			ElementSet result;
			for (const OperationEntry &entry : _operations)
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

			_sets.try_emplace(context, [this, context]() {
				return _generateSet(context);
			});
		}

		void _invalidate(const TContext &context)
		{
			auto it = _sets.find(context);
			if (it != _sets.end())
			{
				it->second.invalidate();
			}
			this->notifyEdition(context);
		}

		void _invalidateAll()
		{
			std::vector<TContext> invalidatedContexts;
			invalidatedContexts.reserve(_sets.size());
			for (auto &[context, set] : _sets)
			{
				set.invalidate();
				invalidatedContexts.push_back(context);
			}

			for (const TContext &context : invalidatedContexts)
			{
				this->notifyEdition(context);
			}
		}

	public:
		Query() = default;

		Query(const Query &) = delete;
		Query &operator=(const Query &) = delete;
		Query(Query &&) = delete;
		Query &operator=(Query &&) = delete;

		template <typename TOperationType, typename... TArgs>
			requires std::derived_from<TOperationType, Operation>
		Query &insert(TArgs &&...args)
		{
			OperationEntry entry;
			entry.operation = std::make_unique<TOperationType>(std::forward<TArgs>(args)...);
			entry.invalidationContract = entry.operation->subscribeToInvalidation(
				[this](const TContext &context) {
					_invalidate(context);
				});
			_operations.push_back(std::move(entry));
			_invalidateAll();
			return *this;
		}

		[[nodiscard]] const ElementSet &elements(const TContext &context) const override
		{
			_ensureSet(context);
			return _sets.at(context);
		}
	};
}
