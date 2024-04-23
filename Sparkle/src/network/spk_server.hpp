#pragma once 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>

#include "network/spk_message.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace spk
{
	class Server
	{
	public:
		using ClientID = size_t;
		using ConnectionCallback = std::function<void(ClientID)>;
		using DisconnectionCallback = std::function<void(ClientID)>;

	private:

		class Acceptator
		{
		private:
			SOCKET serverSocket;
			std::unordered_map<ClientID, SOCKET>& clients;
			std::mutex& clientsMutex;
			ClientID& nextClientId;
			bool isRunning;
			ConnectionCallback& onConnect;
			DisconnectionCallback& onDisconnect;

		public:
			Acceptator(std::unordered_map<ClientID, SOCKET>& clientsMap, std::mutex& mutex, ClientID& clientId,
					ConnectionCallback& connectCallback, DisconnectionCallback& disconnectCallback) :
				clients(clientsMap), clientsMutex(mutex), nextClientId(clientId), onConnect(connectCallback),
				onDisconnect(disconnectCallback), isRunning(false), serverSocket(INVALID_SOCKET) {}

			void start(int port)
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
				serverAddress.sin_port = htons(port);

				if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
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
				while (isRunning)
				{
					struct sockaddr_in clientAddress;
					int clientSize = sizeof(clientAddress);
					SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientSize);
					if (clientSocket == INVALID_SOCKET)
					{
						std::cout << "Accept failed." << std::endl;
					}
					else
					{
						std::lock_guard<std::mutex> lock(clientsMutex);
						ClientID clientId = nextClientId++;
						clients[clientId] = clientSocket;
						std::cout << "Client connected: ID = " << clientId << std::endl;
						if (onConnect)
						{
							onConnect(clientId);
						}
					}
				}
			}

			void stop()
			{
				isRunning = false;
				for (auto& client : clients)
				{
					if (onDisconnect)
					{
						onDisconnect(client.first);
					}
					closesocket(client.second);
				}
				closesocket(serverSocket);
				WSACleanup();
			}
		};

		std::unordered_map<ClientID, SOCKET> clients;
		std::mutex clientsMutex;
		ClientID nextClientId = 0;
		ConnectionCallback onConnect;
		DisconnectionCallback onDisconnect;
		std::unique_ptr<Acceptator> acceptor;
		std::thread serverThread;

	public:
		Server() : acceptor(std::make_unique<Acceptator>(clients, clientsMutex, nextClientId, onConnect, onDisconnect)) {}

		void start(size_t serverPort)
		{
			serverThread = std::thread(&Acceptator::start, acceptor.get(), static_cast<int>(serverPort));
		}

		void stop()
		{
			acceptor->stop();
			if (serverThread.joinable())
			{
				serverThread.join();
			}
		}

		void setConnectionCallback(const ConnectionCallback& callback)
		{
			onConnect = callback;
		}

		void setDisconnectionCallback(const DisconnectionCallback& callback)
		{
			onDisconnect = callback;
		}

		~Server()
		{
			stop();
		}
	};
}