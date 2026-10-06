#pragma once
#include "codec.hpp"
#include <network/replication/protocol.hpp>
#include <network/replication/receiver.hpp>
#include <network/replication/request_queue.hpp>
#include <network/replication/request_service.hpp>
namespace SparkleNetworkExample
{
	// In-memory example using real spk::Message serialization.
	[[nodiscard]] inline State requestedObject()
	{
		using namespace spk::Network;
		Publisher<State> publisher;
		RequestService<State> service(publisher);
		Receiver<State> receiver;
		RequestQueue requests;
		Protocol<State, Codec> protocol(700);
		const auto peer = PeerID::generate(), object = ObjectID::generate();
		const auto session = publisher.open(peer);
		receiver.reset(session);
		(void)requests.reset(session);
		requests.request(object, Clock::time_point{});
		const auto request =
			protocol.decodeRequest(protocol.encode(requests.due({}).front()));
		if (service.receive(peer, request))
		{
			// Application authorization and acquisition/generation belong HERE.
			(void)service.fulfill(peer, request, State{3, 5});
		}
		State local;
		publisher.dispatch({}, 64, [&](PeerID, const Update<State> &update) {
			const auto accepted = receiver.receive(
				protocol.decodeUpdate(protocol.encode(update)));
			if (accepted && accepted->edit == Edit::Set)
			{
				local = *accepted->state;
			}
			return true;
		});
		service.dispatch(64, [&](PeerID, const Reply &reply) {
			return requests.receive(
				protocol.decodeReply(protocol.encode(reply)),
				Clock::time_point{});
		});
		return local;
	}
} // namespace SparkleNetworkExample
