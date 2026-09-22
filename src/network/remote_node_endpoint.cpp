#include "network/remote_node.hpp"

#include "exception.hpp"
#include "network/internal/remote_envelope.hpp"

#include <utility>

namespace spk
{
	void RemoteNode::Endpoint::start(std::uint16_t port)
	{
		_server.start(port);
	}

	void RemoteNode::Endpoint::stop()
	{
		_server.stop();
	}

	std::uint16_t RemoteNode::Endpoint::port() const noexcept
	{
		return _server.port();
	}

	bool RemoteNode::Endpoint::isRunning() const noexcept
	{
		return _server.isRunning();
	}

	void RemoteNode::Endpoint::dispatch()
	{
		_server.messages().drain(_received);
		for (ReceivedMessage &received : _received)
		{
			auto envelope = NetworkInternal::decodeRemoteEnvelope(received.message);
			if (envelope.kind != NetworkInternal::RemoteEnvelopeKind::Request)
			{
				throw Exception("RemoteNode::Endpoint received a non-request envelope.");
			}
			RemoteNode::Endpoint::Request request{
				received.emitter,
				envelope.route,
				std::move(envelope.message)};
			_requests.publish(std::move(request));
		}
	}

	RemoteNode::Endpoint::RequestQueue &RemoteNode::Endpoint::requests() noexcept
	{
		return _requests;
	}

	void RemoteNode::Endpoint::reply(const RemoteNode::Endpoint::Request &request, Message message)
	{
		_server.sendTo(
			request.proxyConnection,
			NetworkInternal::encodeRemoteEnvelope(
				NetworkInternal::RemoteEnvelopeKind::Response,
				request.originConnection,
				message));
	}
}
