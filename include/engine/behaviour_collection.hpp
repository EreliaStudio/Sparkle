#pragma once

#include "engine/behaviour.hpp"
#include "engine/entity_attachment_collection.hpp"

namespace spk
{
	class BehaviourCollection : public EntityAttachmentCollection<Behaviour>
	{
	private:
		using Base = EntityAttachmentCollection<Behaviour>;

	protected:
		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		void registerBehaviour(std::unique_ptr<TBehaviourType> &&behaviour)
		{
			Base::registerAttachment(std::move(behaviour));
		}

		void unregisterBehaviour(Behaviour &behaviour)
		{
			Base::unregisterAttachment(behaviour);
		}

		[[nodiscard]]
		const std::vector<std::unique_ptr<Behaviour>> &behaviours() const noexcept
		{
			return Base::attachments();
		}

	public:
		using OnBehaviourEditionContractProvider =
			typename Base::OnAttachmentEditionContractProvider;
		using OnBehaviourEditionCallback =
			typename Base::OnAttachmentEditionCallback;
		using OnBehaviourEditionContract =
			typename Base::OnAttachmentEditionContract;

		[[nodiscard]] OnBehaviourEditionContract subscribeToBehaviourAddition(
			OnBehaviourEditionCallback callback)
		{
			return Base::subscribeToAttachmentAddition(callback);
		}

		[[nodiscard]] OnBehaviourEditionContract subscribeToBehaviourRemoval(
			OnBehaviourEditionCallback callback)
		{
			return Base::subscribeToAttachmentRemoval(callback);
		}

		template <typename TBehaviourType, typename TPredicate>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] TBehaviourType *getBehaviour(const TPredicate &predicate)
		{
			return Base::template getAttachment<TBehaviourType>(predicate);
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] TBehaviourType *getBehaviour()
		{
			return Base::template getAttachment<TBehaviourType>();
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] TBehaviourType *getBehaviour(const std::regex &regexExpression)
		{
			return Base::template getAttachment<TBehaviourType>(regexExpression);
		}

		template <typename TBehaviourType, typename TPredicate>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] const TBehaviourType *getBehaviour(const TPredicate &predicate) const
		{
			return Base::template getAttachment<TBehaviourType>(predicate);
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] const TBehaviourType *getBehaviour() const
		{
			return Base::template getAttachment<TBehaviourType>();
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] const TBehaviourType *getBehaviour(const std::regex &regexExpression) const
		{
			return Base::template getAttachment<TBehaviourType>(regexExpression);
		}

		template <typename TBehaviourType, typename TPredicate>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<TBehaviourType *> getBehaviours(const TPredicate &predicate)
		{
			return Base::template getAttachments<TBehaviourType>(predicate);
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<TBehaviourType *> getBehaviours()
		{
			return Base::template getAttachments<TBehaviourType>();
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<TBehaviourType *> getBehaviours(const std::regex &regexExpression)
		{
			return Base::template getAttachments<TBehaviourType>(regexExpression);
		}

		template <typename TBehaviourType, typename TPredicate>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<const TBehaviourType *> getBehaviours(const TPredicate &predicate) const
		{
			return Base::template getAttachments<TBehaviourType>(predicate);
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<const TBehaviourType *> getBehaviours() const
		{
			return Base::template getAttachments<TBehaviourType>();
		}

		template <typename TBehaviourType>
			requires std::derived_from<TBehaviourType, Behaviour>
		[[nodiscard]] std::vector<const TBehaviourType *> getBehaviours(const std::regex &regexExpression) const
		{
			return Base::template getAttachments<TBehaviourType>(regexExpression);
		}
	};
}
