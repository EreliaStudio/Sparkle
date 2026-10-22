#pragma once

#include "engine/entity_hierarchy.hpp"
#include "network/server.hpp"

#include <concepts>
#include <exception>
#include <functional>
#include <map>
#include <set>
#include <typeindex>
#include <utility>
#include <vector>

namespace spk
{
	class ServerEntityHierarchy : public EntityHierarchy
	{
	public:
		using EntitySelector = std::function<std::vector<PublishableEntity *>(const Connection &)>;

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
		struct Observation
		{
			Entity *parent;
			InherenceTrait<Entity>::OnChildEditionContract subscription;
		};

		struct Source
		{
			Entity *entity;
			PublishableEntity *publishable;
			EntityTypeID type;
		};

		Server *_server;
		Entity *_root;
		EntitySelector _selector;
		Server::MessageDispatcher::Contract _onRequest;
		std::map<Entity *, Observation> _observed;
		std::map<Entity *, Entity *> _pending;
		std::map<Entity *, UUID> _trackedUUIDs;
		std::map<UUID, Source> _sources;
		std::uint64_t _sequence = 0;
		std::vector<InitializationFailure> _initializationFailures;

		static std::map<PublishableEntity *, ServerEntityHierarchy *> _owners;

		void _withdraw(Entity *entity) noexcept;
		void _observe(Entity *entity, Entity *parent);
		void _resolvePending();
		[[nodiscard]] std::map<UUID, Source> _select(const Connection &connection);
		void _onInitializationRequest(const ReceivedMessage &received);
		void _bind();

	public:
		ServerEntityHierarchy(Server &server, Entity &root, EntitySelector selector, MessageTypes types, Priorities priorities = {});
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
