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

		struct MessageConfiguration
		{
			struct Entry
			{
				Message::Type type;
				Message::Priority priority;
			};

			Entry publication{0, Message::Priority::Low};
			Entry initializationRequest{0, Message::Priority::High};
			Entry initializationResponse{0, Message::Priority::High};
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
		MessageConfiguration _messageConfiguration;
		Entity _root{"EntityHierarchy root"};

	protected:
		std::map<std::type_index, EntityTypeID> _sourceTypes;
		std::map<EntityTypeID, ReplicantFactory> _replicantFactories;

		void _registerSourceType(std::type_index type, EntityTypeID id);
		void _registerReplicantType(EntityTypeID id, ReplicantFactory factory);
		void _clearTypes() noexcept;
		[[nodiscard]] const MessageConfiguration &messageConfiguration() const noexcept;

	public:
		explicit EntityHierarchy(MessageConfiguration configuration);
		virtual ~EntityHierarchy() = default;

		EntityHierarchy(const EntityHierarchy &) = delete;
		EntityHierarchy &operator=(const EntityHierarchy &) = delete;

		[[nodiscard]] Entity &root() noexcept;
		[[nodiscard]] const Entity &root() const noexcept;

		virtual void clear(ClearMode mode) = 0;
	};
}
