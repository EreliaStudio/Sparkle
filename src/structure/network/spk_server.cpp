#include "structure/network/spk_server.hpp"

namespace spk
{
	Server::Acceptator::Acceptator(std::unordered_map<ClientID, SOCKET>& clientsMap, std::mutex& mutex, ClientID& p_clientId,
									ConnectionCallback& connectCallback, DisconnectionCallback& disconnectCallback,
									MessagePool& pool, spk::ThreadSafeQueue<MessageObject>& queue) :
		clients(clientsMap),
		clientsMutex(mutex),
		nextClientId(p_clientId),
		onConnectCallback(connectCallback),
		onDisconnectCallback(disconnectCallback),
		messagePool(pool),
		messageQueue(queue),
		isRunning(false),
		serverSocket(INVALID_SOCKET)
	{
	}

	Server::Acceptator::~Acceptator()
	{
		stop();
	}

	void Server::Acceptator::start(int p_port)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			throw std::runtime_error("Failed to initialize Winsock.");
		}

		serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (serverSocket == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("Cannot create socket.");
		}

		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(p_port);

		if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
		{
			closesocket(serverSocket);
			WSACleanup();
			throw std::runtime_error("Bind failed.");
		}

		if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			closesocket(serverSocket);
			WSACleanup();
			throw std::runtime_error("Listen failed.");
		}

		isRunning = true;
		std::thread(&Acceptator::run, this).detach();
	}

	void Server::Acceptator::run()
	{
		while (isRunning)
		{
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(serverSocket, &readfds);
			int max_fd = static_cast<int>(serverSocket);

			{
				std::lock_guard<std::mutex> lock(clientsMutex);
				for (auto& client : clients)
				{
					FD_SET(client.second, &readfds);
					if (static_cast<int>(client.second) > max_fd)
						max_fd = static_cast<int>(client.second);
				}
			}

			timeval timeout = {0, 50000}; // 50 ms
			int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);

			if (activity == SOCKET_ERROR)
			{
				continue;
			}

			if (FD_ISSET(serverSocket, &readfds))
			{
				acceptNewClient();
			}

			std::vector<ClientID> disconnectedClients;
			{
				std::lock_guard<std::mutex> lock(clientsMutex);
				for (auto& client : clients)
				{
					if (FD_ISSET(client.second, &readfds))
					{
						if (!receiveFromClient(client.first, client.second))
						{
							closesocket(client.second);
							disconnectedClients.push_back(client.first);
						}
					}
				}
			}

			for (auto id : disconnectedClients)
			{
				clients.erase(id);
				onDisconnectCallback(id);
			}
		}
	}

	void Server::Acceptator::acceptNewClient()
	{
		struct sockaddr_in clientAddress;
		int clientSize = sizeof(clientAddress);
		SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientSize);
		if (clientSocket != INVALID_SOCKET)
		{
			std::lock_guard<std::mutex> lock(clientsMutex);
			ClientID clientId = nextClientId++;
			clients[clientId] = clientSocket;
			onConnectCallback(clientId);
		}
	}

	bool Server::Acceptator::receiveFromClient(ClientID p_clientId, SOCKET p_socket)
	{
		auto message = messagePool.obtain();
		const int headerSize = sizeof(spk::Message::Header);

		int bytesRead = ::recv(p_socket, reinterpret_cast<char*>(&message->_header), headerSize, 0);
		if (bytesRead == headerSize)
		{
			if (message->_header.length > 0)
			{
				message->resize(message->_header.length);

				char* dataBuffer = reinterpret_cast<char*>(message->_buffer.data());
				size_t totalBytesReceived = 0;
				while (totalBytesReceived < message->_header.length)
				{
					bytesRead = ::recv(p_socket, dataBuffer + totalBytesReceived, static_cast<int>(message->_header.length) - static_cast<int>(totalBytesReceived), 0);
					if (bytesRead <= 0)
					{
						return false;
					}
					totalBytesReceived += static_cast<int>(bytesRead);
				}
			}
			message->_header.emitterID = p_clientId;
			messageQueue.push(std::move(message));
			return true;
		}

		return false;
	}

	void Server::Acceptator::stop()
	{
		isRunning = false;
		closesocket(serverSocket);
		WSACleanup();
	}

	// Server Methods
	Server::Server() :
		onConnectCallback([&](const ClientID& p_newlyConnectedClient) { spk::cout << "New connection detected: client ID [" << p_newlyConnectedClient << "]" << std::endl; }),
		onDisconnectCallback([&](const ClientID& p_disconnectedClient) { spk::cout << "Client ID [" << p_disconnectedClient << "] disconnected" << std::endl; }),
		acceptor(std::make_unique<Acceptator>(clients, clientsMutex, nextClientId, onConnectCallback, onDisconnectCallback, messagePool, messageQueue))
	{
	}

	void Server::start(size_t p_serverPort)
	{
		acceptor->start(static_cast<int>(p_serverPort));
	}

	void Server::stop()
	{
		acceptor->stop();
	}

	void Server::_internalSendTo(const ClientID& p_clientID, const Message& p_message)
	{
		auto it = clients.find(p_clientID);
		if (it != clients.end())
		{
			SOCKET clientSocket = it->second;
			const int headerSize = sizeof(spk::Message::Header);

			int sentBytes = ::send(clientSocket, reinterpret_cast<const char*>(&(p_message.header())), headerSize, 0);
			if (sentBytes != headerSize)
			{
				spk::cerr << "Failed to send message header." << std::endl;
				return;
			}

			if (p_message.header().length > 0)
			{
				sentBytes = ::send(clientSocket, reinterpret_cast<const char*>(p_message._buffer.data()), static_cast<int>(p_message.header().length), 0);
				if (sentBytes != static_cast<int>(p_message.header().length))
				{
					spk::cerr << "Failed to send message data." << std::endl;
					return;
				}
			}
		}
	}

	void Server::sendTo(const ClientID& p_clientID, const spk::Message& p_message)
	{
		_internalSendTo(p_clientID, p_message);
	}

	void Server::sendTo(const std::vector<ClientID>& p_clients, const spk::Message& p_message)
	{
		for (const auto& emitterID : p_clients)
		{
			_internalSendTo(emitterID, p_message);
		}
	}

	void Server::sendToAll(const spk::Message& p_message)
	{
		std::lock_guard<std::mutex> lock(clientsMutex);
		for (const auto& pair : clients)
		{
			_internalSendTo(pair.first, p_message);
		}
	}

	spk::ThreadSafeQueue<Server::MessageObject>& Server::messages()
	{
		return messageQueue;
	}
}
