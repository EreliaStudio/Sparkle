#pragma once

#include "engine/entity_hierarchy.hpp"
#include "network/client.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace spk
{
	class ClientEntityHierarchy : public EntityHierarchy
	{
	public:
		using ReplicantFactory = std::function<std::unique_ptr<Entity>(const UUID &)>;

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
			std::set<Message::RequestID> outstanding;
		};

		struct SelectionState
		{
			std::map<UUID, UUID> entities;
			std::optional<std::uint64_t> sequence;
		};

		struct PublicationEntry
		{
			UUID parent;
			std::size_t begin;
			std::size_t end;
		};

		std::map<EntityTypeID, ReplicantFactory> _replicantFactories;
		Client *_client = nullptr;
		InitializationConfig _configuration;
		Client::MessageDispatcher::Contract _onPublication;
		Client::MessageDispatcher::Contract _onResponse;
		std::map<UUID, std::unique_ptr<Entity>> _replicas;
		SelectionState _selection;
		std::map<UUID, RequestState> _requests;
		Message::RequestID _nextRequestID = 0;

		void _registerReplicantType(EntityTypeID id, ReplicantFactory factory);
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
