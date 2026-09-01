#pragma once

#include "design_pattern/contract_provider.hpp"

#include <algorithm>
#include <concepts>
#include <memory>
#include <set>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace spk
{
	template <typename TBase>
		requires std::is_polymorphic_v<TBase>
	class PolymorphicContainer
	{
	public:
		using OnElementEditionContractProvider = ContractProvider<TBase &>;
		using OnElementEditionCallback =
			typename OnElementEditionContractProvider::callback_type;
		using OnElementEditionContract =
			typename OnElementEditionContractProvider::Contract;

	private:
		struct ElementCache
		{
			std::set<std::type_index> positiveHits;
			std::set<std::type_index> negativeHits;
		};

		std::vector<std::unique_ptr<TBase>> _elements;
		mutable std::unordered_map<std::type_index, std::vector<TBase *>> _cachedElementsByType;
		mutable std::unordered_map<TBase *, ElementCache> _typeidsPerElement;
		OnElementEditionContractProvider _onElementAdditionContractProvider;
		OnElementEditionContractProvider _onElementRemovalContractProvider;

		void _cachePositiveHit(
			TBase *element,
			const std::type_index &typeIndex) const
		{
			_cachedElementsByType[typeIndex].push_back(element);
			_typeidsPerElement[element].positiveHits.insert(typeIndex);
		}

		void _cacheNegativeHit(
			TBase *element,
			const std::type_index &typeIndex) const
		{
			_typeidsPerElement[element].negativeHits.insert(typeIndex);
		}

		[[nodiscard]] bool _wasParsedAs(
			TBase *element,
			const std::type_index &typeIndex) const
		{
			const ElementCache &cache = _typeidsPerElement.at(element);

			return cache.positiveHits.contains(typeIndex) ||
				   cache.negativeHits.contains(typeIndex);
		}

		void _removeFromTypeCache(
			TBase *element,
			const std::type_index &typeIndex)
		{
			auto it = _cachedElementsByType.find(typeIndex);

			if (it == _cachedElementsByType.end())
			{
				return;
			}

			std::erase(it->second, element);

			if (it->second.empty())
			{
				_cachedElementsByType.erase(it);
			}
		}

		void _removeFromTypeCaches(TBase *element)
		{
			auto it = _typeidsPerElement.find(element);

			if (it == _typeidsPerElement.end())
			{
				return;
			}

			for (const std::type_index &typeIndex : it->second.positiveHits)
			{
				_removeFromTypeCache(element, typeIndex);
			}

			_typeidsPerElement.erase(it);
		}

		void _removeFromElements(TBase *element)
		{
			std::erase_if(
				_elements,
				[element](const std::unique_ptr<TBase> &candidate) {
					return candidate.get() == element;
				});
		}

		template <typename TElementType, typename TPredicate>
		[[nodiscard]] TElementType *_findCachedElement(
			const std::type_index &typeIndex,
			const TPredicate &predicate) const
		{
			auto it = _cachedElementsByType.find(typeIndex);

			if (it == _cachedElementsByType.end())
			{
				return nullptr;
			}

			for (TBase *element : it->second)
			{
				auto *castedElement = static_cast<TElementType *>(element);

				if (predicate(castedElement))
				{
					return castedElement;
				}
			}

			return nullptr;
		}

		template <typename TElementType>
		[[nodiscard]] TElementType *_parseElement(
			TBase *element,
			const std::type_index &typeIndex) const
		{
			if (_wasParsedAs(element, typeIndex))
			{
				return nullptr;
			}

			auto *castedElement = dynamic_cast<TElementType *>(element);

			if (castedElement == nullptr)
			{
				_cacheNegativeHit(element, typeIndex);
			}
			else
			{
				_cachePositiveHit(element, typeIndex);
			}

			return castedElement;
		}

		template <typename TElementType, typename TPredicate>
		[[nodiscard]] TElementType *_findUncachedElement(
			const std::type_index &typeIndex,
			const TPredicate &predicate) const
		{
			for (const auto &element : _elements)
			{
				TElementType *result = _parseElement<TElementType>(
					element.get(),
					typeIndex);

				if (result != nullptr && predicate(result))
				{
					return result;
				}
			}

			return nullptr;
		}

		template <typename TElementType>
		void _parseAllElements(const std::type_index &typeIndex) const
		{
			for (const auto &element : _elements)
			{
				(void)_parseElement<TElementType>(element.get(), typeIndex);
			}
		}

		template <typename TElementType, typename TPredicate>
		[[nodiscard]] std::vector<TElementType *> _findCachedElements(
			const std::type_index &typeIndex,
			const TPredicate &predicate) const
		{
			std::vector<TElementType *> result;
			auto it = _cachedElementsByType.find(typeIndex);

			if (it == _cachedElementsByType.end())
			{
				return result;
			}

			result.reserve(it->second.size());

			for (TBase *element : it->second)
			{
				auto *castedElement = static_cast<TElementType *>(element);

				if (predicate(castedElement))
				{
					result.push_back(castedElement);
				}
			}

			return result;
		}

		template <typename TElementType, typename TPredicate>
		[[nodiscard]] std::vector<TElementType *> _getElements(
			const TPredicate &predicate) const
		{
			const std::type_index typeIndex = typeid(TElementType);
			_parseAllElements<TElementType>(typeIndex);

			return _findCachedElements<TElementType>(typeIndex, predicate);
		}

		template <typename TElementType>
		[[nodiscard]] static std::vector<const TElementType *> _makeConst(
			const std::vector<TElementType *> &elements)
		{
			std::vector<const TElementType *> result;
			result.reserve(elements.size());

			for (TElementType *element : elements)
			{
				result.push_back(element);
			}

			return result;
		}

	protected:
		[[nodiscard]] OnElementEditionContract subscribeToElementAddition(
			OnElementEditionCallback callback)
		{
			return _onElementAdditionContractProvider.subscribe(callback);
		}

		[[nodiscard]] OnElementEditionContract subscribeToElementRemoval(
			OnElementEditionCallback callback)
		{
			return _onElementRemovalContractProvider.subscribe(callback);
		}

		template <typename TElementType>
			requires std::derived_from<TElementType, TBase>
		void registerElement(std::unique_ptr<TElementType> &&element)
		{
			TBase *elementPtr = element.get();

			_typeidsPerElement.emplace(elementPtr, ElementCache{});
			_elements.push_back(std::move(element));
			_onElementAdditionContractProvider.trigger(*elementPtr);
		}

		void unregisterElement(TBase &elementToRemove)
		{
			TBase *element = &elementToRemove;

			if (_typeidsPerElement.contains(element) == false)
			{
				return;
			}

			_onElementRemovalContractProvider.trigger(elementToRemove);
			_removeFromTypeCaches(element);
			_removeFromElements(element);
		}

		[[nodiscard]] const std::vector<std::unique_ptr<TBase>> &elements() noexcept
		{
			return _elements;
		}

		[[nodiscard]] const std::vector<std::unique_ptr<TBase>> &elements() const noexcept
		{
			return _elements;
		}

		template <typename TElementType, typename TPredicate>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] TElementType *getElement(const TPredicate &predicate)
		{
			const std::type_index typeIndex = typeid(TElementType);

			if (auto *result = _findCachedElement<TElementType>(typeIndex, predicate))
			{
				return result;
			}

			return _findUncachedElement<TElementType>(typeIndex, predicate);
		}

		template <typename TElementType>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] TElementType *getElement()
		{
			return getElement<TElementType>(
				[](TElementType *) {
					return true;
				});
		}

		template <typename TElementType, typename TPredicate>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] const TElementType *getElement(const TPredicate &predicate) const
		{
			const std::type_index typeIndex = typeid(TElementType);
			auto wrappedPredicate =
				[&predicate](TElementType *element) {
					return predicate(static_cast<const TElementType *>(element));
				};

			if (auto *result = _findCachedElement<TElementType>(typeIndex, wrappedPredicate))
			{
				return result;
			}

			return _findUncachedElement<TElementType>(typeIndex, wrappedPredicate);
		}

		template <typename TElementType>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] const TElementType *getElement() const
		{
			return getElement<TElementType>(
				[](const TElementType *) {
					return true;
				});
		}

		template <typename TElementType, typename TPredicate>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] std::vector<TElementType *> getElements(const TPredicate &predicate)
		{
			return _getElements<TElementType>(predicate);
		}

		template <typename TElementType>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] std::vector<TElementType *> getElements()
		{
			return getElements<TElementType>(
				[](TElementType *) {
					return true;
				});
		}

		template <typename TElementType, typename TPredicate>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] std::vector<const TElementType *> getElements(const TPredicate &predicate) const
		{
			auto elements = _getElements<TElementType>(
				[&predicate](TElementType *element) {
					return predicate(static_cast<const TElementType *>(element));
				});

			return _makeConst(elements);
		}

		template <typename TElementType>
			requires std::derived_from<TElementType, TBase>
		[[nodiscard]] std::vector<const TElementType *> getElements() const
		{
			return getElements<TElementType>(
				[](const TElementType *) {
					return true;
				});
		}
	};
}
