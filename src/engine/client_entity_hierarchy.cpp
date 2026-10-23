#include "engine/client_entity_hierarchy.hpp"

#include "exception.hpp"

#include <exception>
#include <limits>
#include <utility>

namespace spk
{
	ClientEntityHierarchy::ClientEntityHierarchy(MessageConfiguration messages, InitializationConfig configuration) :
		EntityHierarchy(messages),
		_configuration(configuration)
	{
		setInitializationConfig(configuration);
	}

	ClientEntityHierarchy::ClientEntityHierarchy(MessageConfiguration messages) :
		ClientEntityHierarchy(messages, InitializationConfig{})
	{
	}

	ClientEntityHierarchy::~ClientEntityHierarchy()
	{
		_onPublication.resign();
		_onResponse.resign();
		_replicas.clear();
	}

	void ClientEntityHierarchy::bind(Client &client)
	{
		if (_client != nullptr)
		{
			throw spk::Exception("ClientEntityHierarchy is already bound.");
		}
		auto publication = client.messageDispatcher().subscribeTo(
			messageConfiguration().publication.type,
			[this](const Message &message) {
				_onPublished(message);
			});
		auto response = client.messageDispatcher().subscribeTo(
			messageConfiguration().initializationResponse.type,
			[this](const Message &message) {
				_onInitialized(message);
			});
		_onPublication = std::move(publication);
		_onResponse = std::move(response);
		_client = &client;
	}

	void ClientEntityHierarchy::_registerReplicantType(EntityTypeID id, ReplicantFactory factory)
	{
		if (!factory || _replicantFactories.contains(id))
		{
			throw spk::Exception("Invalid or duplicate EntityHierarchy Replicant factory.");
		}
		_replicantFactories.emplace(id, std::move(factory));
	}

	void ClientEntityHierarchy::registerType(EntityTypeID id, ReplicantFactory factory)
	{
		_registerReplicantType(id, std::move(factory));
	}

	void ClientEntityHierarchy::setInitializationConfig(InitializationConfig configuration)
	{
		if (configuration.maximumRequests == 0 || configuration.delay.count() < 0)
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
		auto selected = _selection.entities.find(uuid);
		auto owned = _replicas.find(uuid);
		if (owned == _replicas.end() || selected == _selection.entities.end())
		{
			return;
		}

		Entity *entity = owned->second.get();
		Entity *parent = nullptr;
		if (selected->second.isNull())
		{
			parent = &root();
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
		for (const auto &[uuid, parent] : _selection.entities)
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
				(state.count == 0 || now - state.lastSent >= _configuration.delay))
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
		Message::Writer writer(messageConfiguration().initializationRequest.type);
		writer.setRequestID(requestID);
		writer << static_cast<std::uint32_t>(pending.size());
		for (const UUID &uuid : pending)
		{
			writer << uuid.bytes();
		}
		Transfer transfer = _client->send(std::move(writer).build(), messageConfiguration().initializationRequest.priority);
		if (transfer.status() == Transfer::Status::Rejected)
		{
			return;
		}
		_nextRequestID = requestID;

		for (const UUID &uuid : pending)
		{
			RequestState &state = _requests[uuid];
			state.outstanding.insert(requestID);
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

		const bool newer = !_selection.sequence || sequence > *_selection.sequence;
		std::exception_ptr firstFailure;
		if (newer)
		{
			_selection.sequence = sequence;
			std::map<UUID, UUID> selection;
			for (const auto &[uuid, entry] : entries)
			{
				selection.emplace(uuid, entry.parent);
			}
			_selection.entities.swap(selection);
			for (auto it = _replicas.begin(); it != _replicas.end();)
			{
				if (!_selection.entities.contains(it->first))
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
				if (_selection.entities.contains(it->first))
				{
					++it;
				}
				else if (it->second.outstanding.empty())
				{
					it = _requests.erase(it);
				}
				else
				{
					it->second.count = 0;
					it->second.lastRequest = 0;
					it->second.blocked = false;
					++it;
				}
			}
			for (const auto &[uuid, parent] : _selection.entities)
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
			if (!_selection.entities.contains(uuid))
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
		auto request = _requests.find(uuid);
		if (request == _requests.end() || !request->second.outstanding.erase(message.requestID()))
		{
			return;
		}
		const bool expired = !_selection.entities.contains(uuid) && request->second.outstanding.empty();
		if (_replicas.contains(uuid))
		{
			if (expired)
			{
				_requests.erase(request);
			}
			return;
		}
		if (outcome != InitializationOutcome::Success)
		{
			if (request->second.lastRequest == message.requestID() &&
				(outcome == InitializationOutcome::Rejected || outcome == InitializationOutcome::TooLarge))
			{
				request->second.blocked = true;
			}
			if (expired)
			{
				_requests.erase(request);
			}
			return;
		}

		if (expired)
		{
			_requests.erase(request);
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
		_selection.entities.clear();
		_requests.clear();
		if (mode == ClearMode::Full)
		{
			_onPublication.resign();
			_onResponse.resign();
			_replicantFactories.clear();
			_client = nullptr;
			_nextRequestID = 0;
			_selection.sequence.reset();
		}
	}
}
