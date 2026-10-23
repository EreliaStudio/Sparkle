#include "engine/server_entity_hierarchy.hpp"

#include "exception.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace spk
{
	ServerEntityHierarchy::ServerEntityHierarchy(Server &server, Entity &root, MessageTypes types, Priorities priorities) :
		EntityHierarchy(types, priorities),
		_server(&server),
		_root(&root)
	{
		_bind();
	}

	ServerEntityHierarchy::~ServerEntityHierarchy()
	{
		_onRequest.resign();
	}

	void ServerEntityHierarchy::_bind()
	{
		_onRequest = _server->messageDispatcher().subscribeTo(
			messageTypes().initializationRequest,
			[this](const ReceivedMessage &message) {
				_onInitializationRequest(message);
			});
	}

	std::map<UUID, ServerEntityHierarchy::Source> ServerEntityHierarchy::_select(const Connection &connection) const
	{
		std::map<UUID, Source> selected;
		for (PublishableEntity *direct : selectEntities(connection))
		{
			if (direct == nullptr || direct->mode() != Publishable::Mode::Source)
			{
				throw spk::Exception("Entity selection returned an invalid Source Entity.");
			}
			Entity *entity = dynamic_cast<Entity *>(direct);
			if (entity == nullptr || entity == _root)
			{
				throw spk::Exception("Entity selection returned a Source outside its hierarchy.");
			}
			for (Entity *current = entity; current != _root; current = current->parent())
			{
				if (current == nullptr || current->parent() == nullptr)
				{
					throw spk::Exception("Selected Source has no valid path to its replication root.");
				}
				auto *source = dynamic_cast<PublishableEntity *>(current);
				if (source == nullptr || source->mode() != Publishable::Mode::Source)
				{
					throw spk::Exception("Selected Source includes a non-publishable or non-Source ancestor.");
				}
				auto type = _sourceTypes.find(std::type_index(typeid(*current)));
				if (type == _sourceTypes.end())
				{
					throw spk::Exception("Unregistered concrete Source Entity type.");
				}
				Entity *parent = current->parent();
				UUID parentUUID = UUID::null();
				if (parent != _root)
				{
					auto *parentSource = dynamic_cast<PublishableEntity *>(parent);
					if (parentSource == nullptr || parentSource->mode() != Publishable::Mode::Source)
					{
						throw spk::Exception("Selected Source includes a non-publishable or non-Source ancestor.");
					}
					parentUUID = parentSource->uuid();
				}
				auto [it, inserted] = selected.emplace(source->uuid(), Source{source, type->second, parentUUID});
				if (!inserted && it->second.publishable != source)
				{
					throw spk::Exception("Duplicate Source Entity UUID in selection.");
				}
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

		std::map<Connection::ID, std::map<UUID, Source>> recipients;
		std::map<UUID, Source> selectedUnion;
		for (const Connection &connection : connections)
		{
			auto selected = _select(connection);
			for (const auto &[uuid, source] : selected)
			{
				auto [it, inserted] = selectedUnion.emplace(uuid, source);
				if (!inserted && it->second.publishable != source.publishable)
				{
					throw spk::Exception("Duplicate Source Entity UUID across recipient selections.");
				}
			}
			recipients.emplace(connection.id(), std::move(selected));
		}

		std::map<UUID, Publishable::UpdateCapture> snapshots;
		for (const auto &[uuid, source] : selectedUnion)
		{
			snapshots.emplace(uuid, source.publishable->captureUpdate(selection));
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
				const ByteStream &partBytes = snapshots.at(uuid).bytes();
				writer << uuid.bytes() << source.parentUUID.bytes() << static_cast<std::uint64_t>(partBytes.size());
				writer.payload().append(partBytes.data().data(), partBytes.size());
			}
			outgoing.emplace(connectionID, std::move(writer).build());
		}

		for (const auto &[uuid, captured] : snapshots)
		{
			selectedUnion.at(uuid).publishable->commitUpdate(captured);
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
		_initializationFailures.clear();
		if (mode == ClearMode::Full)
		{
			_onRequest.resign();
			_clearTypes();
			_server = nullptr;
			_root = nullptr;
		}
	}
}
