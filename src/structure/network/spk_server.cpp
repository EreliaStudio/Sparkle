#include "structure/network/spk_server.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Server::Acceptator::Acceptator(
		std::unordered_map<ClientId, SOCKET> &p_clientsMap,
		std::mutex &p_mutex,
		ClientId &p_clientId,
		MessagePool &p_pool,
		spk::ThreadSafeQueue<MessageObject> &p_queue) :
		_clients(p_clientsMap),
		_clientsMutex(p_mutex),
		_nextClientId(p_clientId),
		_messagePool(p_pool),
		_messageQueue(p_queue),
		_isRunning(false),
		_serverSocket(INVALID_SOCKET)
	{
	}

	Server::Acceptator::~Acceptator()
	{
		stop();
	}

	Server::Contract Server::Acceptator::addOnConnectionCallback(const ConnectionCallback &p_connectionCallback)
	{
		return (_onConnectProvider.subscribe(p_connectionCallback));
	}

	Server::Contract Server::Acceptator::addOnDisconnectionCallback(const DisconnectionCallback &p_disconnectionCallback)
	{
		return (_onDisconnectProvider.subscribe(p_disconnectionCallback));
	}

	void Server::Acceptator::start(int p_port)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			GENERATE_ERROR("Failed to initialize Winsock.");
		}

		_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_serverSocket == INVALID_SOCKET)
		{
			WSACleanup();
			GENERATE_ERROR("Cannot create socket.");
		}

		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(p_port);

		if (bind(_serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
		{
			closesocket(_serverSocket);
			WSACleanup();
			GENERATE_ERROR("Bind failed.");
		}

		if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			closesocket(_serverSocket);
			WSACleanup();
			GENERATE_ERROR("Listen failed.");
		}

		_isRunning = true;
		std::thread(&Acceptator::_run, this).detach();
	}

	int Server::Acceptator::_prepareReadFds(fd_set &p_readfds)
	{
		FD_ZERO(&p_readfds);
		FD_SET(_serverSocket, &p_readfds);
		int maxFD = static_cast<int>(_serverSocket);

		std::lock_guard<std::mutex> lock(_clientsMutex);
		for (auto &client : _clients)
		{
			FD_SET(client.second, &p_readfds);
			if (static_cast<int>(client.second) > maxFD)
			{
				maxFD = static_cast<int>(client.second);
			}
		}

		return maxFD;
	}

	int Server::Acceptator::_waitForActivity(fd_set &p_readfds, int p_maxFd)
	{
		timeval timeout = {0, 50000}; // 50 ms
		return select(p_maxFd + 1, &p_readfds, nullptr, nullptr, &timeout);
	}

	void Server::Acceptator::_handleClients(fd_set &p_readfds)
	{
		std::vector<std::pair<ClientId, SOCKET>> readyClients;
		std::vector<ClientId> disconnectedClients;

		{
			std::lock_guard<std::mutex> lock(_clientsMutex);
			for (auto &client : _clients)
			{
				if (FD_ISSET(client.second, &p_readfds))
				{
					readyClients.push_back(client);
				}
			}
		}

		for (auto &entry : readyClients)
		{
			ClientId id = entry.first;
			SOCKET sock = entry.second;

			if (!_receiveFromClient(id, sock))
			{
				closesocket(sock);
				disconnectedClients.push_back(id);
			}
		}

		{
			std::lock_guard<std::mutex> lock(_clientsMutex);
			for (auto id : disconnectedClients)
			{
				_clients.erase(id);
				_onDisconnectProvider.trigger(id);
			}
		}
	}

	void Server::Acceptator::_run()
	{
		while (_isRunning)
		{
			fd_set readfds;
			int maxFD = _prepareReadFds(readfds);

			int activity = _waitForActivity(readfds, maxFD);
			if (activity == SOCKET_ERROR)
			{
				continue;
			}

			if (FD_ISSET(_serverSocket, &readfds))
			{
				_acceptNewClient();
			}

			_handleClients(readfds);
		}
	}

	void Server::Acceptator::_acceptNewClient()
	{
		struct sockaddr_in clientAddress;
		int clientSize = sizeof(clientAddress);
		SOCKET clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddress, &clientSize);
		if (clientSocket != INVALID_SOCKET)
		{
			std::lock_guard<std::mutex> lock(_clientsMutex);
			ClientId clientId = _nextClientId++;
			_clients[clientId] = clientSocket;
			_onConnectProvider.trigger(clientId);
		}
	}

	bool Server::Acceptator::_receiveFromClient(ClientId p_clientId, SOCKET p_socket)
	{
		auto message = _messagePool.obtain();
		const int headerSize = sizeof(spk::Message::Header);

		int bytesRead = ::recv(p_socket, reinterpret_cast<char *>(&message->_header), headerSize, 0);
		if (bytesRead == headerSize)
		{
			if (message->_header.length > 0)
			{
				message->resize(message->_header.length);

				char *dataBuffer = reinterpret_cast<char *>(message->_buffer.data());
				size_t totalBytesReceived = 0;
				while (totalBytesReceived < message->_header.length)
				{
					bytesRead = ::recv(
						p_socket,
						dataBuffer + totalBytesReceived,
						static_cast<int>(message->_header.length) - static_cast<int>(totalBytesReceived),
						0);
					if (bytesRead <= 0)
					{
						return false;
					}
					totalBytesReceived += static_cast<int>(bytesRead);
				}
			}
			message->_header.emitterID = p_clientId;
			_messageQueue.push(std::move(message));
			return true;
		}

		return false;
	}

	void Server::Acceptator::stop()
	{
		_isRunning = false;
		closesocket(_serverSocket);
		WSACleanup();
	}

	Server::Server() :
		_acceptor(std::make_unique<Acceptator>(_clients, _clientsMutex, _nextClientId, _messagePool, _messageQueue))
	{
	}

	Server::Contract Server::addOnConnectionCallback(const Server::ConnectionCallback &p_connectionCallback)
	{
		return (_acceptor->addOnConnectionCallback(p_connectionCallback));
	}

	Server::Contract Server::addOnDisconnectionCallback(const Server::DisconnectionCallback &p_disconnectionCallback)
	{
		return (_acceptor->addOnDisconnectionCallback(p_disconnectionCallback));
	}

	void Server::start(size_t p_serverPort)
	{
		_acceptor->start(static_cast<int>(p_serverPort));
	}

	void Server::stop()
	{
		_acceptor->stop();
	}

	void Server::_internalSendTo(const ClientId &p_clientID, const Message &p_message)
	{
		auto it = _clients.find(p_clientID);
		if (it != _clients.end())
		{
			SOCKET clientSocket = it->second;
			const int headerSize = sizeof(spk::Message::Header);

			int sentBytes = ::send(clientSocket, reinterpret_cast<const char *>(&(p_message.header())), headerSize, 0);
			if (sentBytes != headerSize)
			{
				spk::cerr() << "Failed to send message header." << std::endl;
				return;
			}

			if (p_message.header().length > 0)
			{
				sentBytes =
					::send(clientSocket, reinterpret_cast<const char *>(p_message._buffer.data()), static_cast<int>(p_message.header().length), 0);
				if (sentBytes != static_cast<int>(p_message.header().length))
				{
					spk::cerr() << "Failed to send message data." << std::endl;
					return;
				}
			}
		}
	}

	void Server::sendTo(const ClientId &p_clientID, const spk::Message &p_message)
	{
		_internalSendTo(p_clientID, p_message);
	}

	void Server::sendTo(const std::vector<ClientId> &p_clients, const spk::Message &p_message)
	{
		for (const auto &emitterID : p_clients)
		{
			_internalSendTo(emitterID, p_message);
		}
	}

	void Server::sendToAll(const spk::Message &p_message)
	{
		std::lock_guard<std::mutex> lock(_clientsMutex);
		for (const auto &pair : _clients)
		{
			_internalSendTo(pair.first, p_message);
		}
	}

	spk::ThreadSafeQueue<Server::MessageObject> &Server::messages()
	{
		return _messageQueue;
	}
}
