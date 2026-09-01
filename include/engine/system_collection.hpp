#pragma once

#include <container/polymorphic_container.hpp>

#include <concepts>
#include <memory>
#include <regex>
#include <utility>
#include <vector>

#include "engine/system.hpp"

namespace spk
{
	class SystemCollection : public PolymorphicContainer<System>
	{
	private:
		using Base = PolymorphicContainer<System>;

	public:
		using OnSystemEditionContractProvider =
			Base::OnElementEditionContractProvider;
		using OnSystemEditionCallback =
			Base::OnElementEditionCallback;
		using OnSystemEditionContract =
			Base::OnElementEditionContract;

	protected:
		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		void registerSystem(std::unique_ptr<TSystemType> &&system)
		{
			Base::registerElement(std::move(system));
		}

		void unregisterSystem(System &system)
		{
			Base::unregisterElement(system);
		}

		[[nodiscard]] const std::vector<std::unique_ptr<System>> &systems() const noexcept
		{
			return Base::elements();
		}

	public:
		[[nodiscard]] OnSystemEditionContract subscribeToSystemAddition(
			OnSystemEditionCallback callback)
		{
			return Base::subscribeToElementAddition(callback);
		}

		[[nodiscard]] OnSystemEditionContract subscribeToSystemRemoval(
			OnSystemEditionCallback callback)
		{
			return Base::subscribeToElementRemoval(callback);
		}

		template <typename TSystemType, typename TPredicate>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] TSystemType *getSystem(const TPredicate &predicate)
		{
			return Base::template getElement<TSystemType>(predicate);
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] TSystemType *getSystem()
		{
			return Base::template getElement<TSystemType>();
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] TSystemType *getSystem(const std::regex &regexExpression)
		{
			return getSystem<TSystemType>(
				[&regexExpression](TSystemType *system) {
					return std::regex_search(system->name(), regexExpression);
				});
		}

		template <typename TSystemType, typename TPredicate>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] const TSystemType *getSystem(const TPredicate &predicate) const
		{
			return Base::template getElement<TSystemType>(predicate);
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] const TSystemType *getSystem() const
		{
			return Base::template getElement<TSystemType>();
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] const TSystemType *getSystem(const std::regex &regexExpression) const
		{
			return getSystem<TSystemType>(
				[&regexExpression](const TSystemType *system) {
					return std::regex_search(system->name(), regexExpression);
				});
		}

		template <typename TSystemType, typename TPredicate>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<TSystemType *> getSystems(const TPredicate &predicate)
		{
			return Base::template getElements<TSystemType>(predicate);
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<TSystemType *> getSystems()
		{
			return Base::template getElements<TSystemType>();
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<TSystemType *> getSystems(const std::regex &regexExpression)
		{
			return getSystems<TSystemType>(
				[&regexExpression](TSystemType *system) {
					return std::regex_search(system->name(), regexExpression);
				});
		}

		template <typename TSystemType, typename TPredicate>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<const TSystemType *> getSystems(const TPredicate &predicate) const
		{
			return Base::template getElements<TSystemType>(predicate);
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<const TSystemType *> getSystems() const
		{
			return Base::template getElements<TSystemType>();
		}

		template <typename TSystemType>
			requires std::derived_from<TSystemType, System>
		[[nodiscard]] std::vector<const TSystemType *> getSystems(const std::regex &regexExpression) const
		{
			return getSystems<TSystemType>(
				[&regexExpression](const TSystemType *system) {
					return std::regex_search(system->name(), regexExpression);
				});
		}
	};
}
