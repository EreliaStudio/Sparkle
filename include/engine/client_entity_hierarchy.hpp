#pragma once

#include "engine/entity_hierarchy.hpp"
#include "network/client.hpp"

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace spk
{
	class ClientEntityHierarchy : public EntityHierarchy
	{
	public:
		struct InitializationConfig
		{
			std::uint32_t maximumRequests = 4;
			std::chrono::milliseconds delay{0};
		};

	private:
		struct RequestState
		{
			std::uint32_t count = 0;
			std::chrono::steady_clock::time_point lastSent{};
			Message::RequestID lastRequest = 0;
			bool blocked = false;
		};

		struct PublicationEntry
		{
			UUID parent;
			std::size_t begin;
			std::size_t end;
		};

		Client *_client = nullptr;
		InitializationConfig _configuration;
		Client::MessageDispatcher::Contract _onPublication;
		Client::MessageDispatcher::Contract _onResponse;
		std::map<UUID, std::unique_ptr<Entity>> _replicas;
		std::map<UUID, UUID> _selection;
		std::map<UUID, RequestState> _requests;
		std::map<Message::RequestID, std::set<UUID>> _batches;
		Message::RequestID _nextRequestID = 0;
		std::uint64_t _sequence = 0;
		bool _hasSelection = false;

		void _onPublished(const Message &message);
		void _onInitialized(const Message &message);
		void _attach(const UUID &uuid);
		void _sendInitializationRequests();
		void _validateParentGraph(const std::map<UUID, PublicationEntry> &entries) const;

	public:
		ClientEntityHierarchy(MessageConfiguration messages, InitializationConfig configuration);
		ClientEntityHierarchy(MessageConfiguration messages);
		~ClientEntityHierarchy() override;

		void bind(Client &client);
		void registerType(EntityTypeID id, ReplicantFactory factory);
		void setInitializationConfig(InitializationConfig configuration);

		[[nodiscard]] Entity *find(const UUID &uuid) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		void clear(ClearMode mode) override;
	};
}
