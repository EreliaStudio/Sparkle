#pragma once

#include "engine/entity_attachment_collection.hpp"
#include "engine/system_participant.hpp"

namespace spk
{
	class SystemParticipantCollection : public EntityAttachmentCollection<System::Participant>
	{
	private:
		using Base = EntityAttachmentCollection<System::Participant>;

	protected:
		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		void registerParticipant(std::unique_ptr<TParticipantType> &&participant)
		{
			Base::registerAttachment(std::move(participant));
		}

		void unregisterParticipant(System::Participant &participant)
		{
			Base::unregisterAttachment(participant);
		}

		[[nodiscard]]
		const std::vector<std::unique_ptr<System::Participant>> &participants() const noexcept
		{
			return Base::attachments();
		}

	public:
		using OnParticipantEditionContractProvider =
			typename Base::OnAttachmentEditionContractProvider;
		using OnParticipantEditionCallback =
			typename Base::OnAttachmentEditionCallback;
		using OnParticipantEditionContract =
			typename Base::OnAttachmentEditionContract;

		[[nodiscard]] OnParticipantEditionContract subscribeToParticipantAddition(
			OnParticipantEditionCallback callback)
		{
			return Base::subscribeToAttachmentAddition(callback);
		}

		[[nodiscard]] OnParticipantEditionContract subscribeToParticipantRemoval(
			OnParticipantEditionCallback callback)
		{
			return Base::subscribeToAttachmentRemoval(callback);
		}

		template <typename TParticipantType, typename TPredicate>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] TParticipantType *getParticipant(const TPredicate &predicate)
		{
			return Base::template getAttachment<TParticipantType>(predicate);
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] TParticipantType *getParticipant()
		{
			return Base::template getAttachment<TParticipantType>();
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] TParticipantType *getParticipant(const std::regex &regexExpression)
		{
			return Base::template getAttachment<TParticipantType>(regexExpression);
		}

		template <typename TParticipantType, typename TPredicate>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] const TParticipantType *getParticipant(const TPredicate &predicate) const
		{
			return Base::template getAttachment<TParticipantType>(predicate);
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] const TParticipantType *getParticipant() const
		{
			return Base::template getAttachment<TParticipantType>();
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] const TParticipantType *getParticipant(const std::regex &regexExpression) const
		{
			return Base::template getAttachment<TParticipantType>(regexExpression);
		}

		template <typename TParticipantType, typename TPredicate>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<TParticipantType *> getParticipants(const TPredicate &predicate)
		{
			return Base::template getAttachments<TParticipantType>(predicate);
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<TParticipantType *> getParticipants()
		{
			return Base::template getAttachments<TParticipantType>();
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<TParticipantType *> getParticipants(const std::regex &regexExpression)
		{
			return Base::template getAttachments<TParticipantType>(regexExpression);
		}

		template <typename TParticipantType, typename TPredicate>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<const TParticipantType *> getParticipants(const TPredicate &predicate) const
		{
			return Base::template getAttachments<TParticipantType>(predicate);
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<const TParticipantType *> getParticipants() const
		{
			return Base::template getAttachments<TParticipantType>();
		}

		template <typename TParticipantType>
			requires std::derived_from<TParticipantType, System::Participant>
		[[nodiscard]] std::vector<const TParticipantType *> getParticipants(const std::regex &regexExpression) const
		{
			return Base::template getAttachments<TParticipantType>(regexExpression);
		}
	};
}
