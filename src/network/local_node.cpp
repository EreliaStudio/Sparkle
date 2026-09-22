#include "network/local_node.hpp"

#include <utility>

namespace spk
{
	void LocalNode::receive(ReceivedMessage message)
	{
		_incoming.publish(std::move(message));
	}

	Node::OutgoingQueue &LocalNode::outgoing() noexcept
	{
		return _outgoing;
	}

	LocalNode::IncomingQueue &LocalNode::incoming() noexcept
	{
		return _incoming;
	}

	void LocalNode::sendTo(ConnectionID connection, Message message)
	{
		_outgoing.publish(OutgoingMessage::to(connection, std::move(message)));
	}

	void LocalNode::reply(const ReceivedMessage &request, Message message)
	{
		sendTo(request.emitter, std::move(message));
	}

	void LocalNode::broadcast(Message message)
	{
		_outgoing.publish(OutgoingMessage::broadcast(std::move(message)));
	}
}
