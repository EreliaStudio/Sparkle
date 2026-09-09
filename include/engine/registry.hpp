#pragma once

#include "design_pattern/trait/query_source_trait.hpp"
#include "engine/contextualizable_trait.hpp"

#include <concepts>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class Registry final : public QuerySourceTrait<TType, TContext>
	{
	public:
		using Base = QuerySourceTrait<TType, TContext>;
		using ElementSet = typename Base::ElementSet;
		using OnEditionContractProvider = spk::ContractProvider<const TContext &, TType *>;
		using OnEditionCallback = typename OnEditionContractProvider::callback_type;
		using OnEditionContract = typename OnEditionContractProvider::Contract;

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
				Registry::instance().add(contextual->context(), object);

				_contextEditionContract = contextual->subscribeToContextEdition(
					[object](const TContext &oldContext, const TContext &newContext) {
						Registry &registry = Registry::instance();
						registry.remove(oldContext, object);
						registry.add(newContext, object);
					});
			}

			~Object()
			{
				TType *object = static_cast<TType *>(this);
				ContextualType *contextual = static_cast<ContextualType *>(object);
				Registry::instance().remove(contextual->context(), object);
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

		std::unordered_map<TContext, Entry> _entries;
		ElementSet _emptyElements;

		Registry() = default;

		bool add(const TContext &context, TType *element)
		{
			Entry &entry = _entries[context];
			const bool inserted = entry.elements.insert(element).second;

			if (inserted)
			{
				entry.onAdditionContractProvider.trigger(context, element);
				this->notifyEdition(context);
			}

			return inserted;
		}

		bool remove(const TContext &context, TType *element)
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
				this->notifyEdition(context);
			}

			return removed;
		}

	public:
		Registry(const Registry &) = delete;
		Registry &operator=(const Registry &) = delete;
		Registry(Registry &&) = delete;
		Registry &operator=(Registry &&) = delete;

		[[nodiscard]] static Registry &instance()
		{
			static Registry instance;
			return instance;
		}

		[[nodiscard]] OnEditionContract subscribeToAddition(
			const TContext &context,
			OnEditionCallback callback)
		{
			return _entries[context].onAdditionContractProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] OnEditionContract subscribeToRemoval(
			const TContext &context,
			OnEditionCallback callback)
		{
			return _entries[context].onRemovalContractProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] const ElementSet &elements(const TContext &context) const override
		{
			auto it = _entries.find(context);
			return it == _entries.end() ? _emptyElements : it->second.elements;
		}
	};
}
