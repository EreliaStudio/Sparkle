#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "network/spk_message.hpp"
#include "data_structure/spk_pool.hpp"
#include "data_structure/spk_thread_safe_deque.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace spk
{
    class Server
    {
    public:
        using ClientID = Message::Header::ClientID;
        using ConnectionCallback = std::function<void(const ClientID&)>;
        using DisconnectionCallback = std::function<void(const ClientID&)>;

		using MessagePool = spk::Pool<spk::Message>;
		using MessageObject = MessagePool::Object;

    private:
        class Acceptator
        {
        private:
            SOCKET serverSocket;
            std::unordered_map<ClientID, SOCKET>& clients;
            std::mutex& clientsMutex;
            ClientID& nextClientId;
            bool isRunning;
            ConnectionCallback& onConnectCallback;
            DisconnectionCallback& onDisconnectCallback;

            MessagePool& messagePool;
            spk::ThreadSafeDeque<MessageObject>& messageQueue;

            std::thread receptionThread;

        public:
            Acceptator(std::unordered_map<ClientID, SOCKET>& clientsMap, std::mutex& mutex, ClientID& p_clientId,
                       ConnectionCallback& connectCallback, DisconnectionCallback& disconnectCallback,
                       MessagePool& pool, spk::ThreadSafeDeque<MessageObject>& queue) :
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

			~Acceptator()
			{
				stop();
			}

            void start(int p_port)
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

            void run()
            {
				std::cout << "Server started and ready to receive new message" << std::endl;
                while (isRunning)
                {
                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(serverSocket, &readfds);
                    int max_fd = serverSocket;

                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        for (auto& client : clients)
                        {
                            FD_SET(client.second, &readfds);
                            if (static_cast<int>(client.second) > max_fd)
                                max_fd = static_cast<int>(client.second);
                        }
                    }

                    timeval timeout;
                    timeout.tv_sec = 0;
                    timeout.tv_usec = 50000; // 50 ms

                    int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

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
				std::cout << "Closing the server" << std::endl;
            }

            void acceptNewClient()
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

            bool receiveFromClient(ClientID p_clientId, SOCKET p_socket)
			{
				auto message = messagePool.obtain();

				const int headerSize = sizeof(spk::Message::Header);
				int bytesRead = ::recv(p_socket, reinterpret_cast<char*>(&message->header()), headerSize, 0);
				if (bytesRead == headerSize)
				{
					if (message->header().length() > 0)
					{
						message->resize(message->header().length());

						char* dataBuffer = reinterpret_cast<char*>(message->buffer().data());
						size_t totalBytesReceived = 0;
						while (totalBytesReceived < message->header().length())
						{
							bytesRead = ::recv(p_socket, dataBuffer + totalBytesReceived, message->header().length() - totalBytesReceived, 0);
							if (bytesRead <= 0)
							{
								return false;
							}
							totalBytesReceived += bytesRead;
						}
					}
                    message->header().clientID = p_clientId;
					messageQueue.push_back(std::move(message));
					return true;
				}
				else if (bytesRead < 0)
				{
					return false;
				}
				else if (bytesRead == 0)
				{
					return false;
				}

				return false;
			}

            void stop()
            {
                isRunning = false;
                closesocket(serverSocket);
                WSACleanup();
            }
        };

        std::unordered_map<ClientID, SOCKET> clients;
        std::mutex clientsMutex;
        ClientID nextClientId = 0;
        ConnectionCallback onConnectCallback;
        DisconnectionCallback onDisconnectCallback;
        std::unique_ptr<Acceptator> acceptor;

        MessagePool messagePool;
        spk::ThreadSafeDeque<MessageObject> messageQueue;


		void _internalSendTo(const ClientID& p_clientID, const Message& p_message)
        {
			auto it = clients.find(p_clientID);
            if (it != clients.end())
            {
				SOCKET clientSocket = it->second;
				int headerSize = sizeof(spk::Message::Header);
				int sentBytes = ::send(clientSocket, reinterpret_cast<const char*>(&(p_message.header())), headerSize, 0);
				if (sentBytes != headerSize)
				{
					std::cerr << "Failed to send message header." << std::endl;
					return;
				}

				if (p_message.header().length() > 0)
				{
					sentBytes = ::send(clientSocket, reinterpret_cast<const char*>(p_message.data()), p_message.header().length(), 0);
					if (sentBytes != static_cast<int>(p_message.header().length()))
					{
						std::cerr << "Failed to send message data." << std::endl;
						return;
					}
				}
			}
		}


    public:
        Server() :
            onConnectCallback([&](const ClientID& p_newlyConnectedClient){std::cout << "New connection detected: client ID [" << p_newlyConnectedClient << "]" << std::endl;}),
            onDisconnectCallback([&](const ClientID& p_disconnectedClient){std::cout << "Client ID [" << p_disconnectedClient << "] disconnected" << std::endl;}),
            acceptor(std::make_unique<Acceptator>(clients, clientsMutex, nextClientId, onConnectCallback, onDisconnectCallback, messagePool, messageQueue))
        {
        }

        void start(size_t p_serverPort)
        {
            acceptor->start(static_cast<int>(p_serverPort));
        }

        void stop()
        {
            acceptor->stop();
        }

		void sendTo(const ClientID& p_clientID, const spk::Message& p_message)
        {
            _internalSendTo(p_clientID, p_message);
        }

        void sendTo(const std::vector<ClientID>& p_clients, const spk::Message& p_message)
        {
            for (const auto& clientID : p_clients)
            {
                _internalSendTo(clientID, p_message);
            }
        }

        void sendToAll(const spk::Message& p_message)
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& pair : clients)
            {
                _internalSendTo(pair.first, p_message);
            }
        }

        spk::ThreadSafeDeque<MessageObject>& messages()
        {
            return messageQueue;
        }
    };
}
