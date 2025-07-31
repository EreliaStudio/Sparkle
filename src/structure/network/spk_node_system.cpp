#include "structure/network/spk_node_system.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void RemoteNode::connect(const std::string &p_address, size_t p_port)
	{
		_client.connect(p_address, p_port);
	}

	void RemoteNode::disconnect()
	{
		_client.disconnect();
	}

	void RemoteNode::treatMessage(spk::Server::MessageObject &&p_message)
	{
		if (_client.isConnected() == false)
		{
			GENERATE_ERROR("Can't send a message through a non-connected RemoteNode.");
		}
		_client.send(*p_message);
	}

	spk::ThreadSafeQueue<spk::Server::MessageObject> &RemoteNode::messages()
	{
		return _client.messages();
	}

	void LocalNode::treatMessage(spk::Server::MessageObject &&p_message)
	{
		_messagesReceived.push(std::move(p_message));
	}

	spk::ThreadSafeQueue<spk::Server::MessageObject> &LocalNode::messageReceived()
	{
		return _messagesReceived;
	}

	spk::Server::MessageObject LocalNode::obtainAwnerMessage(const spk::Message::Header::ClientID &p_emitterID, spk::Message::Header::Type p_type)
	{
		spk::Server::MessageObject result = _messagePool.obtain();
		result->setEmitterID(p_emitterID);
		result->setType(p_type);
		return result;
	}

	spk::Server::MessageObject LocalNode::obtainAwnerMessage(spk::Server::MessageObject &&p_questionMessage, spk::Message::Header::Type p_type)
	{
		spk::Server::MessageObject result = _messagePool.obtain();
		result->setEmitterID(p_questionMessage->header().emitterID);
		result->setType(p_type);
		return result;
	}

	void LocalNode::insertMessageAwnser(spk::Server::MessageObject &&p_messageAwnser)
	{
		_messagesToReturn.push(std::move(p_messageAwnser));
	}

	spk::ThreadSafeQueue<spk::Server::MessageObject> &LocalNode::messages()
	{
		return _messagesToReturn;
	}

	CentralNode::CentralNode()
	{
	}

	void CentralNode::start(size_t p_serverPort)
	{
		_server.start(p_serverPort);
	}

	void CentralNode::addNode(std::string p_nodeName, spk::SafePointer<Node> p_node)
	{
		if (_nodes.contains(p_nodeName))
		{
			GENERATE_ERROR("Can't set a node named [" + p_nodeName + "] : this node already exists.");
		}
		_nodes[p_nodeName] = p_node;
	}

	void CentralNode::setupRedirection(int32_t p_messageType, std::string p_nodeName)
	{
		if (!_nodes.contains(p_nodeName))
		{
			GENERATE_ERROR("Can't set a redirection for message type [" + std::to_string(p_messageType) + "] to node [" + p_nodeName +
						   "] : this node does not exist.");
		}
		_redirections[p_messageType] = _nodes[p_nodeName];
	}

	void CentralNode::redirectMessageToNode()
	{
		while (!_server.messages().empty())
		{
			spk::Server::MessageObject messageToRedirect = _server.messages().pop();
			if (!_redirections.contains(messageToRedirect->header().type))
			{
				GENERATE_ERROR("Unknown message type [" + std::to_string(messageToRedirect->header().type) + "] : no redirection setup.");
			}
			_redirections[messageToRedirect->header().type]->treatMessage(std::move(messageToRedirect));
		}
	}

	void CentralNode::redirectMessageToClients()
	{
		for (const auto &[key, node] : _nodes)
		{
			while (!node->messages().empty())
			{
				spk::Server::MessageObject messageToRedirect = node->messages().pop();
				if (messageToRedirect->header().emitterID != 0)
				{
					_server.sendTo(messageToRedirect->header().emitterID, *messageToRedirect);
				}
				else
				{
					_server.sendToAll(*messageToRedirect);
				}
			}
		}
	}
}
