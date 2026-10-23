#pragma once

#include "engine/entity.hpp"
#include "engine/publishable_entity.hpp"
#include "network/message.hpp"

#include <cstdint>

namespace spk
{
	class EntityHierarchy
	{
	public:
		using EntityTypeID = std::uint32_t;

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
