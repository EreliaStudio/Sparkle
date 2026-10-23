#pragma once

#include "engine/entity_hierarchy.hpp"
#include "network/server.hpp"

#include <concepts>
#include <exception>
#include <map>
#include <typeindex>
#include <utility>
#include <vector>

namespace spk
{
	class ServerEntityHierarchy : public EntityHierarchy
	{
	public:
		struct PublicationResult
		{
			std::map<Connection::ID, Transfer> accepted;
			std::map<Connection::ID, std::exception_ptr> failures;
		};

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

		Server *_server;
		Entity *_root;
		Server::MessageDispatcher::Contract _onRequest;
		std::uint64_t _sequence = 0;
		std::vector<InitializationFailure> _initializationFailures;

		[[nodiscard]] std::map<UUID, Source> _select(const Connection &connection) const;
		void _onInitializationRequest(const ReceivedMessage &received);
		void _bind();

	protected:
		virtual std::vector<PublishableEntity *> selectEntities(const Connection &connection) const = 0;

	public:
		ServerEntityHierarchy(Server &server, Entity &root, MessageTypes types, Priorities priorities = {});
		~ServerEntityHierarchy() override;

		template <typename TEntity>
			requires(std::derived_from<TEntity, Entity> && std::derived_from<TEntity, PublishableEntity>)
		void registerType(EntityTypeID id)
		{
			_registerSourceType(std::type_index(typeid(TEntity)), id);
		}

		[[nodiscard]] PublicationResult publish(
			const ConnectionSet &connections,
			Publishable::UpdateSelection selection = Publishable::UpdateSelection::Modified);

		[[nodiscard]] std::vector<InitializationFailure> takeInitializationFailures();
		void clear(ClearMode mode) override;
	};
}
