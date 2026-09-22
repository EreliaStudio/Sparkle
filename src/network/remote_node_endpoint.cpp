#include "network/remote_node_endpoint.hpp"

#include "exception.hpp"
#include "network/internal/remote_envelope.hpp"

#include <utility>

namespace spk
{
	void RemoteNodeEndpoint::start(std::uint16_t port)
	{
		_server.start(port);
	}

	void RemoteNodeEndpoint::stop()
	{
		_server.stop();
	}

	std::uint16_t RemoteNodeEndpoint::port() const noexcept
	{
		return _server.port();
	}

	bool RemoteNodeEndpoint::isRunning() const noexcept
	{
		return _server.isRunning();
	}

	void RemoteNodeEndpoint::dispatch()
	{
		_server.messages().drain(_received);
		for (ReceivedMessage &received : _received)
		{
			auto envelope = NetworkInternal::decodeRemoteEnvelope(received.message);
			if (envelope.kind != NetworkInternal::RemoteEnvelopeKind::Request)
			{
				throw Exception("RemoteNodeEndpoint received a non-request envelope.");
			}
			_requests.publish(RemoteRequest{
				received.emitter,
				envelope.route,
				std::move(envelope.message)});
		}
	}

	RemoteNodeEndpoint::RequestQueue &RemoteNodeEndpoint::requests() noexcept
	{
		return _requests;
	}

	void RemoteNodeEndpoint::reply(const RemoteRequest &request, Message message)
	{
		_server.sendTo(
			request.proxyConnection,
			NetworkInternal::encodeRemoteEnvelope(
				NetworkInternal::RemoteEnvelopeKind::Response,
				request.originConnection,
				message));
	}
}
