#pragma once

#include "network/spk_server.hpp"

namespace spk
{
	/**
	 * @class Node
	 * @brief Abstract base class representing a node in the network.
	 * 
	 * A node can be either local or remote and is responsible for processing messages.
	 */
	class Node
	{
	public:
		/**
		 * @brief Pure virtual function to treat a message.
		 * @param p_message The message to be treated.
		 */
		virtual void treatMessage(spk::Server::MessageObject&& p_message) = 0;

		/**
		 * @brief Pure virtual function to get the message queue.
		 * @return Reference to the thread-safe message deque.
		 */
		virtual spk::ThreadSafeDeque<spk::Server::MessageObject>& messages() = 0;
	};

	/**
	 * @class RemoteNode
	 * @brief Represents a remote node that connects to a server and sends/receives messages.
	 */
	class RemoteNode : public Node
	{
	private:
		spk::Client _client; ///< The client used to connect to the server.

	public:
		/**
		 * @brief Connects the remote node to a server.
		 * @param p_address The address of the server.
		 * @param p_port The port number of the server.
		 */
		void connect(const std::string& p_address, size_t p_port)
		{
			_client.connect(p_address, p_port);
		}

		/**
		 * @brief Disconnects the remote node from the server.
		 */
		void disconnect()
		{
			_client.disconnect();
		}

		/**
		 * @brief Treats a message by sending it to the server if connected.
		 * @param p_message The message to be treated.
		 * @throw std::runtime_error If the node is not connected.
		 */
		void treatMessage(spk::Server::MessageObject&& p_message)
		{
			if (_client.isConnected() == false)
			{
				throw std::runtime_error("Can't send a message through a non-connected RemoteNode.");
			}
			_client.send(*p_message);
		}

		/**
		 * @brief Gets the message queue of the remote node.
		 * @return Reference to the thread-safe message deque.
		 */
		spk::ThreadSafeDeque<spk::Server::MessageObject>& messages()
		{
			return (_client.messages());
		}
	};

	/**
	 * @class LocalNode
	 * @brief Represents a local node that processes messages locally.
	 */
	class LocalNode : public Node
	{
	private:
		spk::Pool<spk::Message> _messagePool; ///< Pool of messages.
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesReceived; ///< Queue of received messages.
		spk::ThreadSafeDeque<spk::Server::MessageObject> _messagesToReturn; ///< Queue of messages to return.

	public:
		/**
		 * @brief Treats a message by adding it to the received message queue.
		 * @param p_message The message to be treated.
		 */
		void treatMessage(spk::Server::MessageObject&& p_message)
		{
			_messagesReceived.push_back(std::move(p_message));
		}

		/**
		 * @brief Gets the received message queue.
		 * @return Reference to the received message deque.
		 */
		spk::ThreadSafeDeque<spk::Server::MessageObject>& messageReceived()
		{
			return (_messagesReceived);
		}

		/**
		 * @brief Obtains a message object for the owner with a specified emitter ID and type.
		 * @param p_emitterID The emitter ID for the message.
		 * @param p_type The type of the message. Default is -1.
		 * @return The obtained message object.
		 */
		spk::Server::MessageObject obtainAwnerMessage(const spk::Message::Header::ClientID& p_emitterID, spk::Message::Header::Type p_type = -1)
		{
			spk::Server::MessageObject result = _messagePool.obtain();

			result->header().emitterID = p_emitterID;
			result->header().type = p_type;

			return (result);
		}

		/**
		 * @brief Obtains a message object for the owner from a question message with a specified type.
		 * @param p_questionMessage The question message to base the new message on.
		 * @param p_type The type of the new message. Default is -1.
		 * @return The obtained message object.
		 */
		spk::Server::MessageObject obtainAwnerMessage(spk::Server::MessageObject&& p_questionMessage, spk::Message::Header::Type p_type = -1)
		{
			spk::Server::MessageObject result = _messagePool.obtain();

			result->header().emitterID = p_questionMessage->header().emitterID;
			result->header().type = p_type;

			return (result);
		}

		/**
		 * @brief Inserts a message answer into the message return queue.
		 * @param p_messageAwnser The message answer to insert.
		 */
		void insertMessageAwnser(spk::Server::MessageObject&& p_messageAwnser)
		{
			_messagesToReturn.push_back(std::move(p_messageAwnser));
		}

		/**
		 * @brief Gets the message return queue.
		 * @return Reference to the message return deque.
		 */
		spk::ThreadSafeDeque<spk::Server::MessageObject>& messages()
		{
			return (_messagesToReturn);
		}
	};

	/**
	 * @class CentralNode
	 * @brief Represents a central node that manages multiple nodes and redirects messages.
	 */
	class CentralNode
	{
	private:
		spk::Server _server; ///< The server to handle client connections.

		std::map<std::string, Node*> _nodes; ///< Map of node names to node pointers.

		std::map<int32_t, Node*> _redirections; ///< Map of message types to node pointers for redirection.

	public:
		/**
		 * @brief Constructs a CentralNode object.
		 */
		CentralNode()
		{
		}

		/**
		 * @brief Starts the server on a specified port.
		 * @param p_serverPort The port number to start the server on.
		 */
		void start(size_t p_serverPort)
		{
			_server.start(p_serverPort);
		}

		/**
		 * @brief Adds a node to the central node.
		 * @param p_nodeName The name of the node.
		 * @param p_node Pointer to the node to add.
		 * @throw std::runtime_error If a node with the same name already exists.
		 */
		void addNode(std::string p_nodeName, Node* p_node)
		{
			if (_nodes.contains(p_nodeName))
			{
				throw std::runtime_error("Can't set a node named [" + p_nodeName + "] : this node already exists.");
			}
			_nodes[p_nodeName] = p_node;
		}

		/**
		 * @brief Sets up message redirection for a specific message type to a specified node.
		 * @param p_messageType The message type to redirect.
		 * @param p_nodeName The name of the node to redirect to.
		 * @throw std::runtime_error If the node does not exist.
		 */
		void setupRedirection(int32_t p_messageType, std::string p_nodeName)
		{
			if (!_nodes.contains(p_nodeName))
			{
				throw std::runtime_error("Can't set a redirection for message type [" + std::to_string(p_messageType) + "] to node [" + p_nodeName + "] : this node does not exist.");
			}
			_redirections[p_messageType] = _nodes[p_nodeName];
		}

		/**
		 * @brief Redirects messages to the appropriate nodes based on their message types.
		 * @throw std::runtime_error If no redirection is set up for a message type.
		 */
		void redirectMessageToNode()
		{
			while (!_server.messages().empty())
			{
				spk::Server::MessageObject messageToRedirect = _server.messages().pop_front();

				if (!_redirections.contains(messageToRedirect->type()))
				{
					throw std::runtime_error("Unknown message type [" + std::to_string(messageToRedirect->type()) + "] : no redirection setup.");
				}
				_redirections[messageToRedirect->type()]->treatMessage(std::move(messageToRedirect));
			}
		}

		/**
		 * @brief Redirects messages from nodes to the clients.
		 */
		void redirectMessageToClients()
		{
			for (const auto& [key, node] : _nodes)
			{
				while (!node->messages().empty())
				{
					spk::Server::MessageObject messageToRedirect = node->messages().pop_front();

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
	};
}
