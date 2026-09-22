#include "network/remote_node.hpp"

#include "exception.hpp"
#include "network/internal/remote_envelope.hpp"

#include <utility>

namespace spk
{
	bool RemoteNode::isConnected() const noexcept
	{
		return _client.isConnected();
	}

	void RemoteNode::connect(std::string_view address, std::uint16_t port)
	{
		_client.connect(address, port);
	}

	void RemoteNode::disconnect()
	{
		_client.disconnect();
	}

	void RemoteNode::dispatch()
	{
		_forwardRequests();
		_collectResponses();
	}

	void RemoteNode::receive(ReceivedMessage message)
	{
		_incoming.publish(std::move(message));
	}

	Node::OutgoingQueue &RemoteNode::outgoing()
	{
		return _outgoing;
	}

	void RemoteNode::_forwardRequests()
	{
		_incoming.drain(_requests);
		for (ReceivedMessage &message : _requests)
		{
			if (!_client.isConnected())
			{
				throw Exception("Unable to forward through a disconnected RemoteNode.");
			}
			_client.send(
				NetworkInternal::encodeRemoteEnvelope(
					NetworkInternal::RemoteEnvelopeKind::Request,
					message.emitter,
					message.message));
		}
	}

	void RemoteNode::_collectResponses()
	{
		_client.messages().drain(_responses);
		for (Message &message : _responses)
		{
			auto envelope = NetworkInternal::decodeRemoteEnvelope(message);
			if (envelope.kind != NetworkInternal::RemoteEnvelopeKind::Response)
			{
				throw Exception("RemoteNode received a non-response envelope.");
			}
			_outgoing.publish(
				OutgoingMessage::to(
					envelope.route,
					std::move(envelope.message)));
		}
	}
}
