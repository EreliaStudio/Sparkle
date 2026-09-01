#pragma once

#include "engine/contextualizable_trait.hpp"

#include <concepts>
#include <set>
#include <unordered_map>

namespace spk
{
	template <typename TContext, typename TType>
		requires Hashable<TContext>
	class Registry
	{
	public:
		using ElementSet = std::set<TType *>;
		using OnEditionContractProvider = spk::ContractProvider<const TContext &, TType *>;
		using OnEditionCallback = typename OnEditionContractProvider::callback_type;
		using OnEditionContract = typename OnEditionContractProvider::Contract;

		class Query;

		class Object
		{
		private:
			using ContextualType = ContextualizableTrait<TContext>;
			using ContextEditionContract = typename ContextualType::OnContextEditionContract;

			ContextEditionContract _contextEditionContract;

		protected:
			Object()
			{
				static_assert(std::derived_from<TType, ContextualType>);

				TType *object = static_cast<TType *>(this);
				ContextualType *contextual = static_cast<ContextualType *>(object);

				Registry::add(contextual->context(), object);

				_contextEditionContract = contextual->subscribeToContextEdition(
					[object](const TContext &oldContext, const TContext &newContext) {
						Registry::remove(oldContext, object);
						Registry::add(newContext, object);
					});
			}

			~Object()
			{
				TType *object = static_cast<TType *>(this);
				ContextualType *contextual = static_cast<ContextualType *>(object);

				Registry::remove(contextual->context(), object);
			}

			Object(const Object &) = delete;
			Object &operator=(const Object &) = delete;

			Object(Object &&) = delete;
			Object &operator=(Object &&) = delete;
		};

	private:
		struct Entry
		{
			OnEditionContractProvider onAdditionContractProvider;
			OnEditionContractProvider onRemovalContractProvider;
			ElementSet elements;
		};

		static inline std::unordered_map<TContext, Entry> _entries;

		static bool add(const TContext &context, TType *element)
		{
			Entry &entry = _entries[context];

			const bool inserted = entry.elements.insert(element).second;

			if (inserted)
			{
				entry.onAdditionContractProvider.trigger(context, element);
			}

			return inserted;
		}

		static bool remove(const TContext &context, TType *element)
		{
			auto it = _entries.find(context);

			if (it == _entries.end())
			{
				return false;
			}

			const bool removed = it->second.elements.erase(element) != 0;

			if (removed)
			{
				it->second.onRemovalContractProvider.trigger(context, element);
			}

			return removed;
		}

	public:
		static OnEditionContract subscribeToAddition(
			const TContext &context,
			OnEditionCallback callback)
		{
			return _entries[context].onAdditionContractProvider.subscribe(callback);
		}

		static OnEditionContract subscribeToRemoval(
			const TContext &context,
			OnEditionCallback callback)
		{
			return _entries[context].onRemovalContractProvider.subscribe(callback);
		}

		[[nodiscard]] static const ElementSet &elements(const TContext &context)
		{
			static const ElementSet empty;

			auto it = _entries.find(context);

			if (it == _entries.end())
			{
				return empty;
			}

			return it->second.elements;
		}

		[[nodiscard]] static Query query();
	};
}
