#include "engine/client_entity_hierarchy.hpp"

#include "exception.hpp"

#include <exception>
#include <limits>
#include <utility>

namespace spk
{
	ClientEntityHierarchy::ClientEntityHierarchy(
		Client &client,
		Entity &root,
		MessageTypes types,
		InitializationConfig configuration,
		Priorities priorities) :
		EntityHierarchy(types, priorities),
		_client(&client),
		_root(&root),
		_configuration(configuration)
	{
		setInitializationConfig(configuration);
		_bind();
	}

	ClientEntityHierarchy::ClientEntityHierarchy(Client &client, Entity &root, MessageTypes types, Priorities priorities) :
		ClientEntityHierarchy(client, root, types, InitializationConfig{}, priorities)
	{
	}

	ClientEntityHierarchy::~ClientEntityHierarchy()
	{
		_onPublication.resign();
		_onResponse.resign();
		_replicas.clear();
	}

	void ClientEntityHierarchy::_bind()
	{
		_onPublication = _client->messageDispatcher().subscribeTo(
			messageTypes().publication,
			[this](const Message &message) {
				_onPublished(message);
			});
		_onResponse = _client->messageDispatcher().subscribeTo(
			messageTypes().initializationResponse,
			[this](const Message &message) {
				_onInitialized(message);
			});
	}

	void ClientEntityHierarchy::registerType(EntityTypeID id, ReplicantFactory factory)
	{
		_registerReplicantType(id, std::move(factory));
	}

	void ClientEntityHierarchy::setInitializationConfig(InitializationConfig configuration)
	{
		if (configuration.maximumRequests == 0 || configuration.minimumDelay.count() < 0)
		{
			throw spk::Exception("Invalid EntityHierarchy initialization request configuration.");
		}
		_configuration = configuration;
	}

	Entity *ClientEntityHierarchy::find(const UUID &uuid) const noexcept
	{
		auto it = _replicas.find(uuid);
		return it == _replicas.end() ? nullptr : it->second.get();
	}

	std::size_t ClientEntityHierarchy::size() const noexcept
	{
		return _replicas.size();
	}

	void ClientEntityHierarchy::_validateParentGraph(const std::map<UUID, PublicationEntry> &entries) const
	{
		for (const auto &[uuid, entry] : entries)
		{
			std::set<UUID> visited;
			UUID current = uuid;
			while (!current.isNull())
			{
				if (!visited.insert(current).second)
				{
					throw spk::Exception("Circular EntityHierarchy parent graph.");
				}
				auto it = entries.find(current);
				if (it == entries.end())
				{
					throw spk::Exception("EntityHierarchy publication has an unselected parent.");
				}
				current = it->second.parent;
			}
		}
	}

	void ClientEntityHierarchy::_attach(const UUID &uuid)
	{
		auto selected = _selection.find(uuid);
		auto owned = _replicas.find(uuid);
		if (owned == _replicas.end() || selected == _selection.end())
		{
			return;
		}

		Entity *entity = owned->second.get();
		Entity *parent = nullptr;
		if (selected->second.isNull())
		{
			parent = _root;
		}
		else
		{
			auto found = _replicas.find(selected->second);
			if (found != _replicas.end())
			{
				_attach(selected->second);
				parent = found->second.get();
			}
		}

		if (entity->parent() == parent)
		{
			return;
		}
		if (parent == nullptr)
		{
			entity->clearParent();
		}
		else
		{
			entity->setParent(*parent);
		}
	}

	void ClientEntityHierarchy::_sendInitializationRequests()
	{
		if (_client == nullptr)
		{
			throw spk::Exception("ClientEntityHierarchy is no longer bound.");
		}
		std::vector<UUID> pending;
		const auto now = std::chrono::steady_clock::now();
		for (const auto &[uuid, parent] : _selection)
		{
			if (_replicas.contains(uuid))
			{
				continue;
			}
			const auto found = _requests.find(uuid);
			if (found == _requests.end())
			{
				pending.push_back(uuid);
				continue;
			}
			const RequestState &state = found->second;
			if (!state.blocked && state.count < _configuration.maximumRequests &&
				(state.count == 0 || now - state.lastSent >= _configuration.minimumDelay))
			{
				pending.push_back(uuid);
			}
		}
		if (pending.empty())
		{
			return;
		}
		if (_nextRequestID == std::numeric_limits<Message::RequestID>::max() ||
			pending.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw spk::Exception("EntityHierarchy initialization request counter overflow.");
		}
		const Message::RequestID requestID = _nextRequestID + 1;
		Message::Writer writer(messageTypes().initializationRequest);
		writer.setRequestID(requestID);
		writer << static_cast<std::uint32_t>(pending.size());
		for (const UUID &uuid : pending)
		{
			writer << uuid.bytes();
		}
		Transfer transfer = _client->send(std::move(writer).build(), priorities().initializationRequest);
		if (transfer.status() == Transfer::Status::Rejected)
		{
			return;
		}
		_nextRequestID = requestID;
		auto &batch = _batches[requestID];
		for (const UUID &uuid : pending)
		{
			batch.insert(uuid);
			RequestState &state = _requests[uuid];
			++state.count;
			state.lastSent = now;
			state.lastRequest = requestID;
		}
	}

	void ClientEntityHierarchy::_onPublished(const Message &message)
	{
		const auto reader = message.reader();
		std::uint64_t sequence;
		std::uint32_t count;
		reader >> sequence >> count;
		constexpr std::size_t HeaderBytes = UUID::Size * 2 + sizeof(std::uint64_t);
		if (count > reader.remaining() / HeaderBytes)
		{
			throw spk::Exception("Truncated EntitySelection publication.");
		}

		std::map<UUID, PublicationEntry> entries;
		for (std::uint32_t index = 0; index < count; ++index)
		{
			UUID::Storage uuidBytes, parentBytes;
			std::uint64_t length;
			reader >> uuidBytes >> parentBytes >> length;
			if (length > reader.remaining())
			{
				throw spk::Exception("EntitySelection Part payload exceeds message bounds.");
			}
			const UUID uuid(uuidBytes);
			const UUID parent(parentBytes);
			if (uuid.isNull() || !entries.emplace(uuid, PublicationEntry{parent, reader.readOffset(), reader.readOffset() + static_cast<std::size_t>(length)}).second)
			{
				throw spk::Exception("Null or repeated EntitySelection UUID.");
			}
			reader.skip(static_cast<std::size_t>(length));
		}
		if (reader.remaining() != 0)
		{
			throw spk::Exception("Trailing EntitySelection publication bytes.");
		}
		_validateParentGraph(entries);

		const bool newer = !_hasSelection || sequence > _sequence;
		std::exception_ptr firstFailure;
		if (newer)
		{
			_sequence = sequence;
			_hasSelection = true;
			std::map<UUID, UUID> selection;
			for (const auto &[uuid, entry] : entries)
			{
				selection.emplace(uuid, entry.parent);
			}
			_selection.swap(selection);
			for (auto it = _replicas.begin(); it != _replicas.end();)
			{
				if (!_selection.contains(it->first))
				{
					it = _replicas.erase(it);
				}
				else
				{
					++it;
				}
			}
			for (auto it = _requests.begin(); it != _requests.end();)
			{
				if (!_selection.contains(it->first))
				{
					it = _requests.erase(it);
				}
				else
				{
					++it;
				}
			}
			for (const auto &[uuid, parent] : _selection)
			{
				try
				{
					_attach(uuid);
				} catch (...)
				{
					if (!firstFailure)
					{
						firstFailure = std::current_exception();
					}
				}
			}
		}

		for (const auto &[uuid, entry] : entries)
		{
			if (!_selection.contains(uuid))
			{
				continue;
			}
			auto replica = _replicas.find(uuid);
			if (replica == _replicas.end())
			{
				continue;
			}
			if (entry.begin == entry.end)
			{
				continue;
			}
			try
			{
				auto *publishable = dynamic_cast<PublishableEntity *>(replica->second.get());
				publishable->decode(reader.subreader(entry.begin, entry.end));
			} catch (...)
			{
				if (!firstFailure)
				{
					firstFailure = std::current_exception();
				}
			}
		}
		if (newer)
		{
			try
			{
				_sendInitializationRequests();
			} catch (...)
			{
				if (!firstFailure)
				{
					firstFailure = std::current_exception();
				}
			}
		}
		if (firstFailure)
		{
			std::rethrow_exception(firstFailure);
		}
	}

	void ClientEntityHierarchy::_onInitialized(const Message &message)
	{
		const auto reader = message.reader();
		UUID::Storage uuidBytes;
		std::uint8_t outcomeByte;
		reader >> uuidBytes >> outcomeByte;
		const UUID uuid(uuidBytes);
		if (outcomeByte > static_cast<std::uint8_t>(InitializationOutcome::InternalError))
		{
			throw spk::Exception("Invalid EntityHierarchy initialization response outcome.");
		}
		const auto outcome = static_cast<InitializationOutcome>(outcomeByte);
		EntityTypeID type = 0;
		std::size_t begin = 0;
		std::size_t end = 0;
		if (outcome == InitializationOutcome::Success)
		{
			std::uint64_t length;
			reader >> type >> length;
			if (length > reader.remaining())
			{
				throw spk::Exception("Truncated EntityHierarchy initialization response.");
			}
			begin = reader.readOffset();
			end = begin + static_cast<std::size_t>(length);
			reader.skip(static_cast<std::size_t>(length));
		}
		if (reader.remaining() != 0)
		{
			throw spk::Exception("Trailing EntityHierarchy initialization response bytes.");
		}
		auto batch = _batches.find(message.requestID());
		if (batch == _batches.end() || !batch->second.erase(uuid))
		{
			return;
		}
		if (batch->second.empty())
		{
			_batches.erase(batch);
		}
		if (_replicas.contains(uuid))
		{
			return;
		}
		auto request = _requests.find(uuid);
		if (outcome != InitializationOutcome::Success)
		{
			if (request != _requests.end() && request->second.lastRequest == message.requestID() &&
				(outcome == InitializationOutcome::Rejected || outcome == InitializationOutcome::TooLarge))
			{
				request->second.blocked = true;
			}
			return;
		}

		auto factory = _replicantFactories.find(type);
		if (factory == _replicantFactories.end())
		{
			throw spk::Exception("Unregistered EntityHierarchy Replicant type.");
		}
		std::unique_ptr<Entity> entity = factory->second(uuid);
		auto *publishable = entity ? dynamic_cast<PublishableEntity *>(entity.get()) : nullptr;
		if (publishable == nullptr || publishable->mode() != Publishable::Mode::Replicant ||
			publishable->uuid() != uuid || entity->hasParent())
		{
			throw spk::Exception("EntityHierarchy factory must return a detached authoritative Replicant.");
		}
		publishable->decode(reader.subreader(begin, end));
		_replicas.emplace(uuid, std::move(entity));
		_requests.erase(uuid);
		_attach(uuid);
		for (const auto &[child, parent] : _selection)
		{
			if (parent == uuid)
			{
				_attach(child);
			}
		}
	}

	void ClientEntityHierarchy::clear(ClearMode mode)
	{
		if (mode != ClearMode::Runtime && mode != ClearMode::Full)
		{
			throw spk::Exception("Invalid EntityHierarchy clear mode.");
		}
		_replicas.clear();
		_selection.clear();
		_requests.clear();
		_batches.clear();
		if (mode == ClearMode::Full)
		{
			_onPublication.resign();
			_onResponse.resign();
			_clearTypes();
			_client = nullptr;
			_root = nullptr;
		}
	}
}
