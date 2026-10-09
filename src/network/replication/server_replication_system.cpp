#include "network/replication/server_replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <utility>

namespace spk::Network
{
	ServerReplicationSystem::ServerReplicationSystem(spk::Message::Type type) :
		ReplicationSystem(type)
	{
	}

	ServerReplicationSystem::~ServerReplicationSystem()
	{
		unbind();
	}

	void ServerReplicationSystem::bind(spk::Server &server)
	{
		if (_server == &server)
			return;
		unbind();
		auto interest = server.messageDispatcher().subscribeTo(interestUpdateType(),
			[this](const spk::ReceivedMessage &incoming) { onInterestUpdate(incoming); });
		auto removal = server.messageDispatcher().subscribeTo(interestRemovalType(),
			[this](const spk::ReceivedMessage &incoming) { onInterestRemoval(incoming); });
		auto connected = server.subscribeToConnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.insert(peer);
		});
		auto disconnected = server.subscribeToDisconnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.erase(peer);
			_interests.erase(peer);
			_sent.erase(peer);
		});
		_interestContract = std::move(interest);
		_removalContract = std::move(removal);
		_connectionContract = std::move(connected);
		_disconnectionContract = std::move(disconnected);
		_server = &server;
	}

	void ServerReplicationSystem::unbind()
	{
		_interestContract.resign();
		_removalContract.resign();
		_connectionContract.resign();
		_disconnectionContract.resign();
		_server = nullptr;
		const std::scoped_lock lock(_peerMutex);
		_peers.clear();
		_interests.clear();
		_sent.clear();
		_elapsed = Interval{};
	}

	bool ServerReplicationSystem::isBound() const noexcept
	{
		return _server != nullptr;
	}

	void ServerReplicationSystem::setRequestAuthorizer(Authorizer authorizer)
	{
		_authorizer = std::move(authorizer);
	}

	void ServerReplicationSystem::setInterestEvaluator(std::shared_ptr<const InterestEvaluator> evaluator)
	{
		_evaluator = std::move(evaluator);
	}

	void ServerReplicationSystem::setRefreshInterval(Interval interval)
	{
		if (interval <= Interval::zero())
			throw spk::Exception("Replication refresh interval must be positive.");
		_refreshInterval = interval;
		_elapsed = Interval{};
	}

	ServerReplicationSystem::Interval ServerReplicationSystem::refreshInterval() const noexcept
	{
		return _refreshInterval;
	}

	std::unique_ptr<Interest> ServerReplicationSystem::_createInterest(const spk::Message::Reader &)
	{
		return nullptr;
	}

	void ServerReplicationSystem::onInterestUpdate(const spk::ReceivedMessage &incoming)
	{
		auto reader = incoming.message.reader();
		if (reader.size() < 2 * sizeof(spk::UUID::Storage))
			return;
		spk::UUID::Storage bytes{};
		reader >> bytes;
		const spk::UUID identifier(bytes);
		if (identifier.isNull())
			return;
		auto interest = _createInterest(reader);
		if (!interest || !_evaluator || reader.readOffset() != reader.size())
			return;
		const std::scoped_lock lock(_peerMutex);
		if (!_peers.contains(incoming.emitter))
			return;
		auto &entry = _interests[incoming.emitter][identifier];
		entry.interest = std::move(interest);
		entry.visible.clear();
	}

	void ServerReplicationSystem::onInterestRemoval(const spk::ReceivedMessage &incoming)
	{
		auto reader = incoming.message.reader();
		if (reader.size() != sizeof(spk::UUID::Storage))
			return;
		spk::UUID::Storage bytes{};
		reader >> bytes;
		const std::scoped_lock lock(_peerMutex);
		auto peer = _interests.find(incoming.emitter);
		if (peer == _interests.end())
			return;
		peer->second.erase(spk::UUID(bytes));
		_sent[incoming.emitter].clear();
	}

	spk::Message ServerReplicationSystem::stateMessage(const ServerReplicatedComponent &component) const
	{
		spk::Message::Writer writer(stateType());
		writer << component.identifier().bytes() << component.version();
		component.capture(writer);
		return std::move(writer).build();
	}

	void ServerReplicationSystem::publishUpdates()
	{
		if (_server == nullptr || engine() == nullptr || !_evaluator)
			return;
		const auto &all = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
		std::map<spk::UUID, spk::Message> messages;
		const std::scoped_lock lock(_peerMutex);
		for (spk::ConnectionID peer : _peers)
		{
			std::set<spk::UUID> visible;
			for (spk::Component *item : all)
			{
				auto *component = dynamic_cast<ServerReplicatedComponent *>(item);
				if (!component || !_authorizer || !_authorizer(peer, *component))
					continue;
				for (auto &[id, subscription] : _interests[peer])
				{
					if (_evaluator->matches(*subscription.interest, *component, peer))
					{
						visible.insert(component->identifier());
						break;
					}
				}
				if (!visible.contains(component->identifier()))
					continue;
				auto &sent = _sent[peer];
				auto previous = sent.find(component->identifier());
				if (previous != sent.end() && previous->second == component->version())
					continue;
				auto message = messages.find(component->identifier());
				if (message == messages.end())
					message = messages.emplace(component->identifier(), stateMessage(*component)).first;
				_server->sendTo(peer, message->second);
				sent[component->identifier()] = component->version();
			}
			auto &sent = _sent[peer];
			for (auto it = sent.begin(); it != sent.end();)
			{
				if (visible.contains(it->first))
				{
					++it;
					continue;
				}
				spk::Message::Writer writer(componentRemovalType());
				writer << it->first.bytes();
				_server->sendTo(peer, std::move(writer).build());
				it = sent.erase(it);
			}
		}
	}

	void ServerReplicationSystem::_updateState(spk::UpdateContext &context)
	{
		if (_server == nullptr)
			return;
		_server->treatMessages();
		_elapsed += context.deltaTime;
		if (_elapsed < _refreshInterval)
			return;
		_elapsed = Interval{};
		publishUpdates();
	}
}
