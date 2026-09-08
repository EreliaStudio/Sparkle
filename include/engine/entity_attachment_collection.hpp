#pragma once

#include <container/polymorphic_container.hpp>

#include <concepts>
#include <memory>
#include <regex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/entity_attachment.hpp"

namespace spk
{
	template <typename TAttachmentBase>
		requires std::derived_from<TAttachmentBase, EntityAttachment>
	class EntityAttachmentCollection : public PolymorphicContainer<TAttachmentBase>
	{
	private:
		using Base = PolymorphicContainer<TAttachmentBase>;

	public:
		using OnAttachmentEditionContractProvider =
			typename Base::OnElementEditionContractProvider;
		using OnAttachmentEditionCallback =
			typename Base::OnElementEditionCallback;
		using OnAttachmentEditionContract =
			typename Base::OnElementEditionContract;

	private:
		using AttachmentNameEditionContract = NameTrait::NameEditionContract;

		OnAttachmentEditionContractProvider _onAttachmentNameEditionContractProvider;
		std::unordered_map<TAttachmentBase *, AttachmentNameEditionContract> _attachmentNameEditionContracts;

	protected:
		[[nodiscard]] OnAttachmentEditionContract subscribeToAttachmentAddition(
			OnAttachmentEditionCallback callback)
		{
			return Base::subscribeToElementAddition(callback);
		}

		[[nodiscard]] OnAttachmentEditionContract subscribeToAttachmentRemoval(
			OnAttachmentEditionCallback callback)
		{
			return Base::subscribeToElementRemoval(callback);
		}

		[[nodiscard]] OnAttachmentEditionContract subscribeToAttachmentNameEdition(
			OnAttachmentEditionCallback callback)
		{
			return _onAttachmentNameEditionContractProvider.subscribe(std::move(callback));
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		void registerAttachment(std::unique_ptr<TAttachmentType> &&attachment)
		{
			TAttachmentBase *observed = attachment.get();
			auto nameContract = observed->subscribeToNameEdition([this, observed](const std::string &) {
				_onAttachmentNameEditionContractProvider.trigger(*observed);
			});
			_attachmentNameEditionContracts.emplace(observed, std::move(nameContract));
			try
			{
				Base::registerElement(std::move(attachment));
			} catch (...)
			{
				if (attachment != nullptr)
				{
					_attachmentNameEditionContracts.erase(observed);
				}
				throw;
			}
		}

		void unregisterAttachment(TAttachmentBase &attachment)
		{
			TAttachmentBase *observed = &attachment;
			Base::unregisterElement(attachment);
			_attachmentNameEditionContracts.erase(observed);
		}

		[[nodiscard]] const std::vector<std::unique_ptr<TAttachmentBase>> &attachments() const noexcept
		{
			return Base::elements();
		}

		template <typename TAttachmentType, typename TPredicate>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] TAttachmentType *getAttachment(const TPredicate &predicate)
		{
			return Base::template getElement<TAttachmentType>(predicate);
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] TAttachmentType *getAttachment()
		{
			return Base::template getElement<TAttachmentType>();
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] TAttachmentType *getAttachment(const std::regex &regexExpression)
		{
			return getAttachment<TAttachmentType>(
				[&regexExpression](TAttachmentType *attachment) {
					return std::regex_search(attachment->name(), regexExpression);
				});
		}

		template <typename TAttachmentType, typename TPredicate>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] const TAttachmentType *getAttachment(const TPredicate &predicate) const
		{
			return Base::template getElement<TAttachmentType>(predicate);
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] const TAttachmentType *getAttachment() const
		{
			return Base::template getElement<TAttachmentType>();
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] const TAttachmentType *getAttachment(const std::regex &regexExpression) const
		{
			return getAttachment<TAttachmentType>(
				[&regexExpression](const TAttachmentType *attachment) {
					return std::regex_search(attachment->name(), regexExpression);
				});
		}

		template <typename TAttachmentType, typename TPredicate>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<TAttachmentType *> getAttachments(const TPredicate &predicate)
		{
			return Base::template getElements<TAttachmentType>(predicate);
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<TAttachmentType *> getAttachments()
		{
			return Base::template getElements<TAttachmentType>();
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<TAttachmentType *> getAttachments(const std::regex &regexExpression)
		{
			return getAttachments<TAttachmentType>(
				[&regexExpression](TAttachmentType *attachment) {
					return std::regex_search(attachment->name(), regexExpression);
				});
		}

		template <typename TAttachmentType, typename TPredicate>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<const TAttachmentType *> getAttachments(const TPredicate &predicate) const
		{
			return Base::template getElements<TAttachmentType>(predicate);
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<const TAttachmentType *> getAttachments() const
		{
			return Base::template getElements<TAttachmentType>();
		}

		template <typename TAttachmentType>
			requires std::derived_from<TAttachmentType, TAttachmentBase>
		[[nodiscard]] std::vector<const TAttachmentType *> getAttachments(const std::regex &regexExpression) const
		{
			return getAttachments<TAttachmentType>(
				[&regexExpression](const TAttachmentType *attachment) {
					return std::regex_search(attachment->name(), regexExpression);
				});
		}
	};
}
