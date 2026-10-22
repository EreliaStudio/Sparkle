#include "engine/server_entity_hierarchy.hpp"

#include "exception.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace spk
{
	std::map<PublishableEntity *, ServerEntityHierarchy *> ServerEntityHierarchy::_owners;

	ServerEntityHierarchy::ServerEntityHierarchy(Server &server, Entity &root, EntitySelector selector, MessageTypes types, Priorities priorities) :
		EntityHierarchy(types, priorities),
		_server(&server),
		_root(&root),
		_selector(std::move(selector))
	{
		if (!_selector)
		{
			throw spk::Exception("ServerEntityHierarchy requires an EntitySelector.");
		}
		_pending.emplace(_root, nullptr);
		_bind();
	}

	ServerEntityHierarchy::~ServerEntityHierarchy()
	{
		_onRequest.resign();
		for (const auto &[uuid, source] : _sources)
		{
			_owners.erase(source.publishable);
		}
	}

	void ServerEntityHierarchy::_bind()
	{
		_onRequest = _server->messageDispatcher().subscribeTo(
			messageTypes().initializationRequest,
			[this](const ReceivedMessage &message) {
				_onInitializationRequest(message);
			});
	}

	void ServerEntityHierarchy::_withdraw(Entity *entity)
	{
		for (auto it = _pending.begin(); it != _pending.end();)
		{
			if (it->first == entity || it->second == entity)
			{
				it = _pending.erase(it);
			}
			else
			{
				++it;
			}
		}

		std::vector<Entity *> descendants;
		for (const auto &[tracked, observation] : _observed)
		{
			if (observation.parent == entity)
			{
				descendants.push_back(tracked);
			}
		}
		for (Entity *child : descendants)
		{
			_withdraw(child);
		}

		auto uuidIt = _trackedUUIDs.find(entity);
		if (uuidIt != _trackedUUIDs.end())
		{
			auto sourceIt = _sources.find(uuidIt->second);
			if (sourceIt != _sources.end())
			{
				_owners.erase(sourceIt->second.publishable);
				_sources.erase(sourceIt);
			}
			_trackedUUIDs.erase(uuidIt);
		}
		_observed.erase(entity);
	}

	void ServerEntityHierarchy::_observe(Entity *entity, Entity *parent)
	{
		if (_observed.contains(entity))
		{
			return;
		}

		PublishableEntity *source = nullptr;
		EntityTypeID typeID = 0;
		if (entity != _root)
		{
			source = dynamic_cast<PublishableEntity *>(entity);
			if (source == nullptr || source->mode() != Publishable::Mode::Source)
			{
				throw spk::Exception("Replication subtree contains a non-Source or non-publishable Entity.");
			}
			auto type = _sourceTypes.find(std::type_index(typeid(*entity)));
			if (type == _sourceTypes.end())
			{
				throw spk::Exception("Unregistered concrete Source Entity type.");
			}
			typeID = type->second;
			if (auto existing = _owners.find(source); existing != _owners.end() && existing->second != this)
			{
				throw spk::Exception("Source Entity is already tracked by another ServerEntityHierarchy.");
			}
			if (_sources.contains(source->uuid()))
			{
				throw spk::Exception("Duplicate Source Entity UUID in hierarchy.");
			}
		}

		auto contract = entity->subscribeToChildEdition([this, entity](Entity *child, InherenceTrait<Entity>::ChildEdition edition) {
			if (edition == InherenceTrait<Entity>::ChildEdition::Added)
			{
				_pending.insert_or_assign(child, entity);
			}
			else
			{
				_withdraw(child);
			}
		});
		_observed.emplace(entity, Observation{parent, std::move(contract)});
		if (source != nullptr)
		{
			_owners.emplace(source, this);
			_sources.emplace(source->uuid(), Source{entity, source, typeID});
			_trackedUUIDs.emplace(entity, source->uuid());
		}
		for (Entity *child : entity->children())
		{
			if (!_observed.contains(child))
			{
				_pending.emplace(child, entity);
			}
		}
	}

	void ServerEntityHierarchy::_resolvePending()
	{
		while (!_pending.empty())
		{
			const auto it = _pending.begin();
			Entity *entity = it->first;
			Entity *parent = it->second;
			_observe(entity, parent);
			_pending.erase(entity);
		}
	}

	std::map<UUID, ServerEntityHierarchy::Source> ServerEntityHierarchy::_select(const Connection &connection)
	{
		std::map<UUID, Source> selected;
		for (PublishableEntity *direct : _selector(connection))
		{
			if (direct == nullptr || direct->mode() != Publishable::Mode::Source)
			{
				throw spk::Exception("EntitySelector returned an invalid Source Entity.");
			}
			Entity *entity = dynamic_cast<Entity *>(direct);
			if (entity == nullptr || !_trackedUUIDs.contains(entity))
			{
				throw spk::Exception("EntitySelector returned a Source outside its hierarchy.");
			}
			for (Entity *current = entity; current != _root; current = current->parent())
			{
				if (current == nullptr)
				{
					throw spk::Exception("EntitySelector Source has no valid path to its replication root.");
				}
				auto tracked = _trackedUUIDs.find(current);
				if (tracked == _trackedUUIDs.end())
				{
					throw spk::Exception("EntitySelector includes a non-publishable ancestor.");
				}
				selected.emplace(tracked->second, _sources.at(tracked->second));
			}
		}
		return selected;
	}

	ServerEntityHierarchy::PublicationResult ServerEntityHierarchy::publish(
		const ConnectionSet &connections,
		Publishable::UpdateSelection selection)
	{
		PublicationResult result;
		if (_server == nullptr)
		{
			throw spk::Exception("ServerEntityHierarchy is no longer bound.");
		}
		if (connections.empty())
		{
			return result;
		}
		if (_sequence == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("EntityHierarchy publication sequence overflow.");
		}
		_resolvePending();
		std::map<Connection::ID, std::map<UUID, Source>> recipients;
		std::set<UUID> selectedUnion;
		for (const Connection &connection : connections)
		{
			auto selected = _select(connection);
			for (const auto &[uuid, source] : selected)
			{
				selectedUnion.insert(uuid);
			}
			recipients.emplace(connection.id(), std::move(selected));
		}

		std::map<UUID, Publishable::UpdateCapture> snapshots;
		for (const UUID &uuid : selectedUnion)
		{
			snapshots.emplace(uuid, _sources.at(uuid).publishable->captureUpdate(selection));
		}

		const std::uint64_t nextSequence = _sequence + 1;
		std::map<Connection::ID, Message> outgoing;
		for (const auto &[connectionID, selected] : recipients)
		{
			if (selected.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("EntitySelection exceeds the publication wire count.");
			}
			Message::Writer writer(messageTypes().publication);
			writer << nextSequence << static_cast<std::uint32_t>(selected.size());
			for (const auto &[uuid, source] : selected)
			{
				UUID parentUUID = UUID::null();
				Entity *parent = source.entity->parent();
				if (parent != nullptr && parent != _root)
				{
					parentUUID = _trackedUUIDs.at(parent);
				}
				const ByteStream &partBytes = snapshots.at(uuid).bytes();
				writer << uuid.bytes() << parentUUID.bytes() << static_cast<std::uint64_t>(partBytes.size());
				writer.payload().append(partBytes.data().data(), partBytes.size());
			}
			outgoing.emplace(connectionID, std::move(writer).build());
		}

		for (const auto &[uuid, captured] : snapshots)
		{
			_sources.at(uuid).publishable->commitUpdate(captured);
		}
		_sequence = nextSequence;
		for (const Connection &connection : connections)
		{
			try
			{
				result.accepted.emplace(connection.id(), _server->sendTo(connection, outgoing.at(connection.id()), priorities().publication));
			} catch (...)
			{
				result.failures.emplace(connection.id(), std::current_exception());
			}
		}
		return result;
	}

	void ServerEntityHierarchy::_onInitializationRequest(const ReceivedMessage &received)
	{
		const auto reader = received.message.reader();
		std::uint32_t count;
		reader >> count;
		if (count > reader.remaining() / UUID::Size)
		{
			throw spk::Exception("Truncated EntityHierarchy initialization request.");
		}
		std::vector<UUID> requested;
		requested.reserve(count);
		for (std::uint32_t index = 0; index < count; ++index)
		{
			UUID::Storage bytes;
			reader >> bytes;
			requested.emplace_back(bytes);
		}
		if (reader.remaining() != 0)
		{
			throw spk::Exception("Trailing EntityHierarchy initialization request bytes.");
		}

		_resolvePending();
		Connection connection = _server->connection(received.emitter);
		auto authorized = _select(connection);
		for (const UUID &uuid : requested)
		{
			try
			{
				auto makeResponse = [&](InitializationOutcome outcome, EntityTypeID typeID, const ByteStream &bytes) {
					Message::Writer writer(messageTypes().initializationResponse);
					writer.setRequestID(received.message.requestID());
					writer << uuid.bytes() << static_cast<std::uint8_t>(outcome);
					if (outcome == InitializationOutcome::Success)
					{
						writer << typeID << static_cast<std::uint64_t>(bytes.size());
						writer.payload().append(bytes.data().data(), bytes.size());
					}
					return std::move(writer).build();
				};
				Message response = makeResponse(InitializationOutcome::Rejected, 0, {});
				auto found = authorized.find(uuid);
				if (found != authorized.end())
				{
					try
					{
						const Source &source = found->second;
						const ByteStream bytes = source.publishable->encodeInitialization();
						response = makeResponse(InitializationOutcome::Success, source.type, bytes);
						if (response.size() > _server->maxMessageBytes())
						{
							response = makeResponse(InitializationOutcome::TooLarge, 0, {});
						}
					} catch (...)
					{
						_initializationFailures.push_back({uuid, std::current_exception()});
						response = makeResponse(InitializationOutcome::InternalError, 0, {});
					}
				}
				(void)_server->sendTo(connection, response, priorities().initializationResponse);
			} catch (...)
			{
				_initializationFailures.push_back({uuid, std::current_exception()});
			}
		}
	}

	std::vector<ServerEntityHierarchy::InitializationFailure> ServerEntityHierarchy::takeInitializationFailures()
	{
		std::vector<InitializationFailure> result;
		result.swap(_initializationFailures);
		return result;
	}

	void ServerEntityHierarchy::clear(ClearMode mode)
	{
		if (mode != ClearMode::Runtime && mode != ClearMode::Full)
		{
			throw spk::Exception("Invalid EntityHierarchy clear mode.");
		}
		for (const auto &[uuid, source] : _sources)
		{
			_owners.erase(source.publishable);
		}
		_observed.clear();
		_pending.clear();
		_trackedUUIDs.clear();
		_sources.clear();
		_initializationFailures.clear();
		if (mode == ClearMode::Full)
		{
			_onRequest.resign();
			_clearTypes();
			_server = nullptr;
			_root = nullptr;
		}
		else
		{
			_pending.emplace(_root, nullptr);
		}
	}
}
