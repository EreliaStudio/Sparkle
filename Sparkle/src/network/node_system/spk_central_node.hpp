#pragma once

#include "network/spk_server.hpp"

namespace spk
{
	class Node
	{
	private:

	public:
		virtual void treatMessage(spk::Server::MessageObject&& p_message) = 0;
		virtual spk::ThreadSafeDeque<spk::Server::MessageObject>& messages() = 0;
	};

	class RemoteNode : public Node
	{
	private:
		spk::Client _client;

	public:
		void connect(const std::string& p_address, size_t p_port)
		{
			_client.connect(p_address, p_port);
		}

		void disconnect()
		{
			_client.disconnect();
		}

		void treatMessage(spk::Server::MessageObject&& p_message)
		{
			if (_client.isConnected() == false)
			{
                spk::throwException("Can't send a message thought a non-connected RemoteNode");
			}
			_client.send(*p_message);
		}

		spk::ThreadSafeDeque<spk::Server::MessageObject>& messages()
		{
			return (_client.messages());
		}
	};

	class LocalNode : public Node
	{
	private:
		spk::Pool<spk::Message> _messagePool;
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesReceived;
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesToReturn;

	public:
		void treatMessage(spk::Server::MessageObject&& p_message)
		{
            DEBUG_LINE();
			_messagesReceived.push_back(std::move(p_message));
		}

		spk::ThreadSafeDeque<spk::Server::MessageObject>& messageReceived()
		{
			return (_messagesReceived);
		}

		spk::Server::MessageObject obtainAwnerMessage(const spk::Message::Header::ClientID& p_emitterID, spk::Message::Header::Type p_type = -1)
		{
			spk::Server::MessageObject result = _messagePool.obtain();

			result->header().emitterID = p_emitterID;
			result->header().type = p_type;

			return (result);
		}

		spk::Server::MessageObject obtainAwnerMessage(spk::Server::MessageObject&& p_questionMessage, spk::Message::Header::Type p_type = -1)
		{
			spk::Server::MessageObject result = _messagePool.obtain();

			result->header().emitterID = p_questionMessage->header().emitterID;
			result->header().type = p_type;

			return (result);
		}

		void insertMessageAwnser(spk::Server::MessageObject&& p_messageAwnser)
		{
			_messagesToReturn.push_back(std::move(p_messageAwnser));
		}

		spk::ThreadSafeDeque<spk::Server::MessageObject>& messages()
		{
			return (_messagesToReturn);
		}
	};

	class CentralNode
	{
	private:
		spk::Server _server;

		std::map<std::string, Node*> _nodes;

		std::map<int32_t, Node*> _redirections;

	public:
		CentralNode()
		{

		}

		void start(size_t p_serverPort)
		{
			_server.start(p_serverPort);
		}

		void addNode(std::string p_nodeName, Node* p_node)
		{
			if (_nodes.contains(p_nodeName) == true)
			{
				spk::throwException("Can't set a node named [" + p_nodeName + "] : this node already exist");
			}
			std::cout << "Adding node [" << p_nodeName << "](" << p_node << ")" << std::endl;
			_nodes[p_nodeName] = p_node;
		}

		void setupRedirection(int32_t p_messageType, std::string p_nodeName)
		{
			if (_nodes.contains(p_nodeName) == false)
			{
				spk::throwException("Can't set a redirection for message Type [" + std::to_string(p_messageType) + "] to node [" + p_nodeName + "] : this node does not exist");
			}
			std::cout << "Setting up type " << p_messageType << " to node " << p_nodeName << "(" << _nodes[p_nodeName] << ")" << std::endl;
			_redirections[p_messageType] = _nodes[p_nodeName];
		}

		void redirectMessageToNode()
		{
			while (_server.messages().empty() == false)
			{
				spk::Server::MessageObject messageToRedirect = _server.messages().pop_front();

				if (_redirections.contains(messageToRedirect->type()) == false)
				{
					spk::throwException("Unknow message type [" + std::to_string(messageToRedirect->type()) + "] : no redirection setup");
				}
				std::cout << "Redirecting message [" << messageToRedirect->type() << "] to node (" << _redirections[messageToRedirect->type()] << ")" << std::endl;
				_redirections[messageToRedirect->type()]->treatMessage(std::move(messageToRedirect));
			}	
		}

		void redirectMessageToClients()
		{
			for (const auto& [key, node] : _nodes)
			{
				while (node->messages().empty() == false)
				{
					spk::Server::MessageObject messageToRedirect = node->messages().pop_front();

					if (messageToRedirect->header().emitterID != 0)
						_server.sendTo(messageToRedirect->header().emitterID, *messageToRedirect);
					else
						_server.sendToAll(*messageToRedirect);
				}
			}
		}
	};
}