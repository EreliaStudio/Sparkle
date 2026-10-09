#include "network/replication/client_replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <utility>

namespace spk::Network
{
	void ClientReplicationSystem::SubscriptionState::invalidate() noexcept
	{
		const std::scoped_lock lock(mutex);
		for (auto &[identifier, weak] : subscriptions)
		{
			if (auto entry = weak.lock())
			{
				entry->valid = false;
			}
		}
		subscriptions.clear();
		client = nullptr;
	}

	ClientReplicationSystem::Subscription::Subscription(
		std::shared_ptr<SubscriptionState> owner, std::shared_ptr<SubscriptionEntry> entry) :
		_owner(owner),
		_entry(std::move(entry))
	{
	}

	ClientReplicationSystem::Subscription &ClientReplicationSystem::Subscription::operator=(
		Subscription &&other) noexcept
	{
		if (this != &other)
		{
			cancel();
			_owner = std::move(other._owner);
			_entry = std::move(other._entry);
		}
		return *this;
	}

	ClientReplicationSystem::Subscription::~Subscription()
	{
		cancel();
	}

	bool ClientReplicationSystem::Subscription::isValid() const noexcept
	{
		auto owner = _owner.lock();
		if (!owner || !_entry)
		{
			return false;
		}
		const std::scoped_lock lock(owner->mutex);
		return _entry->valid && owner->client != nullptr;
	}

	void ClientReplicationSystem::Subscription::update(const Interest &interest)
	{
		auto owner = _owner.lock();
		if (!owner || !_entry)
		{
			throw spk::Exception("Interest subscription is no longer valid.");
		}
		const std::scoped_lock lock(owner->mutex);
		if (!_entry->valid || owner->client == nullptr)
		{
			throw spk::Exception("Interest subscription is no longer valid.");
		}
		spk::Message::Writer writer(owner->updateType);
		writer << _entry->identifier.bytes() << interest.type().bytes();
		interest.serialize(writer);
		owner->client->send(std::move(writer).build());
	}

	void ClientReplicationSystem::Subscription::cancel() noexcept
	{
		auto owner = _owner.lock();
		if (owner && _entry)
		{
			const std::scoped_lock lock(owner->mutex);
			if (_entry->valid && owner->client != nullptr)
			{
				try
				{
					spk::Message::Writer writer(owner->removalType);
					writer << _entry->identifier.bytes();
					owner->client->send(std::move(writer).build());
				} catch (...)
				{
					// The connection may be closing; the server clears its subscriptions on disconnect.
				}
			}
			_entry->valid = false;
			owner->subscriptions.erase(_entry->identifier);
		}
		_entry.reset();
		_owner.reset();
	}

	ClientReplicationSystem::ClientReplicationSystem(spk::Message::Type type) :
		ReplicationSystem(type)
	{
		_subscriptions->updateType = interestUpdateType();
		_subscriptions->removalType = interestRemovalType();
	}

	ClientReplicationSystem::~ClientReplicationSystem()
	{
		unbind();
	}

	void ClientReplicationSystem::bind(spk::Client &client)
	{
		if (_client == &client)
		{
			return;
		}
		unbind();
		auto state = client.messageDispatcher().subscribeTo(stateType(), [this](const spk::Message &message) {
			onState(message);
		});
		auto removal = client.messageDispatcher().subscribeTo(componentRemovalType(), [this](const spk::Message &message) {
			onComponentRemoval(message);
		});
		auto disconnected = client.subscribeToDisconnection([this] {
			_subscriptions->invalidate();
			_resetPending.store(true, std::memory_order_release);
		});
		auto connected = client.subscribeToConnection([this, &client] {
			{
				const std::scoped_lock lock(_subscriptions->mutex);
				_subscriptions->client = &client;
			}
			_resetPending.store(true, std::memory_order_release);
		});
		_stateContract = std::move(state);
		_componentRemovalContract = std::move(removal);
		_disconnectionContract = std::move(disconnected);
		_connectionContract = std::move(connected);
		_client = &client;
		{
			const std::scoped_lock lock(_subscriptions->mutex);
			_subscriptions->client = &client;
		}
		_resetPending.store(true, std::memory_order_release);
	}

	void ClientReplicationSystem::unbind()
	{
		_stateContract.resign();
		_componentRemovalContract.resign();
		_disconnectionContract.resign();
		_connectionContract.resign();
		_subscriptions->invalidate();
		_client = nullptr;
		_resetPending.store(true, std::memory_order_release);
	}

	ClientReplicationSystem::Subscription ClientReplicationSystem::subscribe(const Interest &interest)
	{
		if (_client == nullptr || !_client->isConnected())
		{
			throw spk::Exception("Client must be connected to subscribe to an interest.");
		}
		auto entry = std::make_shared<SubscriptionEntry>();
		entry->identifier = spk::UUID::generate();
		Subscription result(_subscriptions, entry);
		{
			const std::scoped_lock lock(_subscriptions->mutex);
			_subscriptions->subscriptions.emplace(entry->identifier, entry);
		}
		try
		{
			result.update(interest);
		} catch (...)
		{
			result.cancel();
			throw;
		}
		return result;
	}

	void ClientReplicationSystem::resetReceivedRevisions()
	{
		if (engine() == nullptr)
		{
			return;
		}
		const auto &components = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
		for (spk::Component *item : components)
		{
			if (auto *component = dynamic_cast<ClientReplicatedComponent *>(item))
			{
				component->resetReceivedRevision();
			}
		}
	}

	bool ClientReplicationSystem::isBound() const noexcept
	{
		return _client != nullptr;
	}

	void ClientReplicationSystem::onState(const spk::Message &message)
	{
		auto reader = message.payload().reader();
		if (reader.size() < sizeof(spk::UUID::Storage) + sizeof(std::uint64_t))
		{
			return;
		}
		spk::UUID::Storage bytes{};
		std::uint64_t revision = 0;
		reader >> bytes >> revision;
		auto *component = dynamic_cast<ClientReplicatedComponent *>(find(spk::UUID(bytes)));
		if (component != nullptr)
		{
			component->apply(reader, revision);
		}
	}

	void ClientReplicationSystem::onComponentRemoval(const spk::Message &message)
	{
		auto reader = message.reader();
		if (reader.size() != sizeof(spk::UUID::Storage))
		{
			return;
		}
		spk::UUID::Storage bytes{};
		reader >> bytes;
		auto *component = dynamic_cast<ClientReplicatedComponent *>(find(spk::UUID(bytes)));
		if (component != nullptr)
		{
			component->leaveInterest();
		}
	}

	void ClientReplicationSystem::_updateState(spk::UpdateContext &)
	{
		if (_resetPending.exchange(false, std::memory_order_acq_rel))
		{
			resetReceivedRevisions();
		}
		if (_client != nullptr)
		{
			_client->treatMessages();
		}
	}
}
