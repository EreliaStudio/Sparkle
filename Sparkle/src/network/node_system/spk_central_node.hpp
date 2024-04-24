#pragma once

#include "network/spk_server.hpp"

namespace spk
{
	class Node
	{
	private:

	public:
		virtual void treatMessage(const spk::Server::MessageObject& p_message) = 0;
		virtual spk::ThreadSafeDeque<spk::Server::MessageObject>& messages() = 0;
	};

	class RemoteNode
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
			_client.send(*p_message);
		}

		spk::ThreadSafeDeque<spk::Server::MessageObject>& messages()
		{
			return (_client.messages());
		}
	};

	class LocalNode
	{
	private:
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesReceived;
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesToReturn;

	public:
		void treatMessage(spk::Server::MessageObject&& p_message)
		{
			_messagesReceived.push_back(std::move(p_message));
		}

		spk::ThreadSafeDeque<spk::Server::MessageObject>& messageToParse()
		{
			return (_messagesReceived);
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
				throw std::runtime_error("Can't set a node named [" + p_nodeName + "] : this node already exist");
			}
			_nodes[p_nodeName] = p_node;
		}

		void setupRedirection(int32_t p_messageType, std::string p_nodeName)
		{
			if (_nodes.contains(p_nodeName) == false)
			{
				throw std::runtime_error("Can't set a redirection for message Type [" + std::to_string(p_messageType) + "] to node [" + p_nodeName + "] : this node does not exist");
			}
			_redirections[p_messageType] = _nodes[p_nodeName];
		}

		void redirectMessageToNode()
		{
			while (_server.messages().empty() == false)
			{
				spk::Server::MessageObject messageToRedirect = _server.messages().pop_front();

				if (_redirections.contains(messageToRedirect->header().type) == false)
				{
					throw std::runtime_error("Unknow message type [" + std::to_string(messageToRedirect->header().type) + "] : no redirection setup");
				}
				_redirections[messageToRedirect->header().type]->treatMessage(messageToRedirect);
			}	
		}

		void redirectMessageToClients()
		{
			for (const auto& [key, node] : _nodes)
			{
				while (node->messages().empty() == false)
				{
					spk::Server::MessageObject messageToRedirect = node->messages().pop_front();

					if (messageToRedirect->header().clientID != 0)
						_server.sendTo(messageToRedirect->header().clientID, *messageToRedirect);
					else
						_server.sendToAll(*messageToRedirect);
				}
			}
		}
	};
}