#pragma once

#include "engine/entity_attachment_collection.hpp"
#include "engine/component.hpp"

namespace spk
{
	class ComponentCollection : public EntityAttachmentCollection<Component>
	{
	private:
		using Base = EntityAttachmentCollection<Component>;

	protected:
		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		void registerComponent(std::unique_ptr<TComponentType> &&component)
		{
			Base::registerAttachment(std::move(component));
		}

		void unregisterComponent(Component &component)
		{
			Base::unregisterAttachment(component);
		}

		[[nodiscard]]
		const std::vector<std::unique_ptr<Component>> &components() const noexcept
		{
			return Base::attachments();
		}

	public:
		using OnComponentEditionContractProvider =
			typename Base::OnAttachmentEditionContractProvider;
		using OnComponentEditionCallback =
			typename Base::OnAttachmentEditionCallback;
		using OnComponentEditionContract =
			typename Base::OnAttachmentEditionContract;
		using OnComponentNameEditionContract =
			typename Base::OnAttachmentEditionContract;

		[[nodiscard]] OnComponentEditionContract subscribeToComponentAddition(
			OnComponentEditionCallback callback)
		{
			return Base::subscribeToAttachmentAddition(callback);
		}

		[[nodiscard]] OnComponentEditionContract subscribeToComponentRemoval(
			OnComponentEditionCallback callback)
		{
			return Base::subscribeToAttachmentRemoval(callback);
		}

		[[nodiscard]] OnComponentNameEditionContract subscribeToComponentNameEdition(
			OnComponentEditionCallback callback)
		{
			return Base::subscribeToAttachmentNameEdition(std::move(callback));
		}

		template <typename TComponentType, typename TPredicate>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] TComponentType *getComponent(const TPredicate &predicate)
		{
			return Base::template getAttachment<TComponentType>(predicate);
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] TComponentType *getComponent()
		{
			return Base::template getAttachment<TComponentType>();
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] TComponentType *getComponent(const std::regex &regexExpression)
		{
			return Base::template getAttachment<TComponentType>(regexExpression);
		}

		template <typename TComponentType, typename TPredicate>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] const TComponentType *getComponent(const TPredicate &predicate) const
		{
			return Base::template getAttachment<TComponentType>(predicate);
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] const TComponentType *getComponent() const
		{
			return Base::template getAttachment<TComponentType>();
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] const TComponentType *getComponent(const std::regex &regexExpression) const
		{
			return Base::template getAttachment<TComponentType>(regexExpression);
		}

		template <typename TComponentType, typename TPredicate>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<TComponentType *> getComponents(const TPredicate &predicate)
		{
			return Base::template getAttachments<TComponentType>(predicate);
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<TComponentType *> getComponents()
		{
			return Base::template getAttachments<TComponentType>();
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<TComponentType *> getComponents(const std::regex &regexExpression)
		{
			return Base::template getAttachments<TComponentType>(regexExpression);
		}

		template <typename TComponentType, typename TPredicate>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<const TComponentType *> getComponents(const TPredicate &predicate) const
		{
			return Base::template getAttachments<TComponentType>(predicate);
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<const TComponentType *> getComponents() const
		{
			return Base::template getAttachments<TComponentType>();
		}

		template <typename TComponentType>
			requires std::derived_from<TComponentType, Component>
		[[nodiscard]] std::vector<const TComponentType *> getComponents(const std::regex &regexExpression) const
		{
			return Base::template getAttachments<TComponentType>(regexExpression);
		}
	};
}
