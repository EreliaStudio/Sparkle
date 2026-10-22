#pragma once

#include "engine/entity.hpp"
#include "engine/publishable_entity.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <typeindex>

namespace spk
{
	class EntityHierarchy
	{
	public:
		using EntityTypeID = std::uint32_t;
		using ReplicantFactory = std::function<std::unique_ptr<Entity>(const UUID &)>;

		struct MessageTypes
		{
			Message::Type publication;
			Message::Type initializationRequest;
			Message::Type initializationResponse;
		};

		struct Priorities
		{
			Message::Priority publication = Message::Priority::Low;
			Message::Priority initializationRequest = Message::Priority::High;
			Message::Priority initializationResponse = Message::Priority::High;
		};

		enum class ClearMode
		{
			Runtime,
			Full
		};

		enum class InitializationOutcome : std::uint8_t
		{
			Success,
			Rejected,
			TooLarge,
			InternalError
		};

	private:
		MessageTypes _messageTypes;
		Priorities _priorities;

	protected:
		std::map<std::type_index, EntityTypeID> _sourceTypes;
		std::map<EntityTypeID, ReplicantFactory> _replicantFactories;

		void _registerSourceType(std::type_index type, EntityTypeID id);
		void _registerReplicantType(EntityTypeID id, ReplicantFactory factory);
		void _clearTypes() noexcept;
		[[nodiscard]] const MessageTypes &messageTypes() const noexcept;
		[[nodiscard]] const Priorities &priorities() const noexcept;

	public:
		explicit EntityHierarchy(MessageTypes types, Priorities priorities = {});
		virtual ~EntityHierarchy() = default;

		EntityHierarchy(const EntityHierarchy &) = delete;
		EntityHierarchy &operator=(const EntityHierarchy &) = delete;

		virtual void clear(ClearMode mode) = 0;
	};
}
