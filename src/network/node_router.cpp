#include "network/node_router.hpp"

#include "exception.hpp"

#include <utility>

namespace spk
{
	void NodeRouter::start(std::uint16_t port)
	{
		_server.start(port);
	}

	void NodeRouter::stop()
	{
		_server.stop();
	}

	void NodeRouter::addNode(std::string name, Node &node)
	{
		if (_nodes.contains(name))
		{
			throw Exception("A network node named [" + name + "] already exists.");
		}
		for (const auto &[existingName, existingNode] : _nodes)
		{
			if (existingNode == &node)
			{
				throw Exception("The same network node cannot be registered twice.");
			}
		}
		_nodes.emplace(std::move(name), &node);
	}

	void NodeRouter::removeNode(std::string_view name)
	{
		auto iterator = _nodes.find(std::string(name));
		if (iterator == _nodes.end())
		{
			throw Exception("Unknown network node [" + std::string(name) + "].");
		}

		Node *node = iterator->second;
		_nodes.erase(iterator);
		for (auto route = _redirections.begin(); route != _redirections.end();)
		{
			route = route->second == node ? _redirections.erase(route) : std::next(route);
		}
	}

	void NodeRouter::redirect(Message::Type messageType, std::string_view nodeName)
	{
		_redirections[messageType] = &_node(nodeName);
	}

	void NodeRouter::removeRedirection(Message::Type messageType)
	{
		_redirections.erase(messageType);
	}

	void NodeRouter::dispatch()
	{
		_dispatchIncoming();
		_dispatchOutgoing();
	}

	Server &NodeRouter::server() noexcept
	{
		return _server;
	}

	const Server &NodeRouter::server() const noexcept
	{
		return _server;
	}

	Node &NodeRouter::_node(std::string_view name) const
	{
		const auto iterator = _nodes.find(std::string(name));
		if (iterator == _nodes.end())
		{
			throw Exception("Unknown network node [" + std::string(name) + "].");
		}
		return *iterator->second;
	}

	void NodeRouter::_dispatchIncoming()
	{
		_server.messages().drain(_incomingBuffer);
		for (ReceivedMessage &received : _incomingBuffer)
		{
			const auto iterator = _redirections.find(received.message.type());
			if (iterator == _redirections.end())
			{
				throw Exception("No node redirection exists for message type [" + std::to_string(received.message.type()) + "].");
			}
			iterator->second->receive(std::move(received));
		}
	}

	void NodeRouter::_dispatchOutgoing()
	{
		for (const auto &[name, node] : _nodes)
		{
			_dispatchOutgoing(*node);
		}
	}

	void NodeRouter::_dispatchOutgoing(Node &node)
	{
		node.outgoing().drain(_outgoingBuffer);
		for (const OutgoingMessage &outgoing : _outgoingBuffer)
		{
			if (outgoing.recipient.has_value())
			{
				_server.sendTo(*outgoing.recipient, outgoing.message);
				continue;
			}
			_server.sendToAll(outgoing.message);
		}
	}
}
