#pragma once

#include "engine/entity_hierarchy.hpp"
#include "network/server.hpp"

#include <concepts>
#include <exception>
#include <expected>
#include <map>
#include <typeindex>
#include <utility>
#include <vector>

namespace spk
{
	class ServerEntityHierarchy : public EntityHierarchy
	{
	public:
		using PublicationResult = std::map<Connection::ID, std::expected<Transfer, std::exception_ptr>>;

		struct InitializationFailure
		{
			UUID uuid;
			std::exception_ptr error;
		};

	private:
		struct Source
		{
			PublishableEntity *publishable;
			EntityTypeID type;
			UUID parentUUID;
		};

		std::map<std::type_index, EntityTypeID> _sourceTypes;
		Server *_server = nullptr;
		Server::MessageDispatcher::Contract _onRequest;
		std::uint64_t _sequence = 0;
		std::vector<InitializationFailure> _initializationFailures;

		void _registerSourceType(std::type_index type, EntityTypeID id);
		[[nodiscard]] std::map<UUID, Source> _select(const Connection &connection) const;
		void _onInitializationRequest(const ReceivedMessage &received);

	protected:
		virtual std::vector<PublishableEntity *> selectEntities(const Connection &connection) const = 0;

	public:
		ServerEntityHierarchy(MessageConfiguration messages);
		~ServerEntityHierarchy() override;

		template <typename TEntity>
			requires(std::derived_from<TEntity, Entity> && std::derived_from<TEntity, PublishableEntity>)
		void registerType(EntityTypeID id)
		{
			_registerSourceType(std::type_index(typeid(TEntity)), id);
		}

		void bind(Server &server);

		[[nodiscard]] PublicationResult publish(
			const ConnectionSet &connections,
			Publishable::UpdateSelection selection = Publishable::UpdateSelection::Modified);

		[[nodiscard]] std::vector<InitializationFailure> takeInitializationFailures();
		void clear(ClearMode mode) override;
	};
}
