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
    /**
     * @class Server
     * @brief Represents a multi-client server that handles connections, disconnections, and message passing.
     * 
     * This class provides the functionality to create a server that can accept multiple client connections,
     * receive messages from clients, and send messages to one or more clients. It uses Winsock for networking
     * and supports thread-safe operations.
     */
    class Server
    {
    public:
        using ClientID = Message::Header::ClientID; ///< Type alias for Client ID.
        using ConnectionCallback = std::function<void(const ClientID&)>; ///< Callback type for client connection.
        using DisconnectionCallback = std::function<void(const ClientID&)>; ///< Callback type for client disconnection.
        using MessagePool = spk::Pool<spk::Message>; ///< Type alias for a pool of messages.
        using MessageObject = MessagePool::Object; ///< Type alias for a message object from the pool.

    private:
        /**
         * @class Acceptator
         * @brief Handles accepting new client connections and receiving messages from connected clients.
         */
        class Acceptator
        {
        private:
            SOCKET serverSocket; ///< The server socket for accepting connections.
            std::unordered_map<ClientID, SOCKET>& clients; ///< Reference to the map of client sockets.
            std::mutex& clientsMutex; ///< Reference to the mutex for thread-safe access to the client map.
            ClientID& nextClientId; ///< Reference to the next client ID to assign.
            bool isRunning; ///< Flag indicating if the server is running.
            ConnectionCallback& onConnectCallback; ///< Callback for client connections.
            DisconnectionCallback& onDisconnectCallback; ///< Callback for client disconnections.
            MessagePool& messagePool; ///< Reference to the message pool.
            spk::ThreadSafeDeque<MessageObject>& messageQueue; ///< Reference to the thread-safe message queue.
            std::thread receptionThread; ///< Thread for handling client reception.

        public:
            /**
             * @brief Constructs an Acceptator object.
             * @param clientsMap Reference to the map of client sockets.
             * @param mutex Reference to the mutex for client map access.
             * @param p_clientId Reference to the next client ID to assign.
             * @param connectCallback Reference to the connection callback.
             * @param disconnectCallback Reference to the disconnection callback.
             * @param pool Reference to the message pool.
             * @param queue Reference to the thread-safe message queue.
             */
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

            /**
             * @brief Destructor for Acceptator. Stops the server if running.
             */
            ~Acceptator()
            {
                stop();
            }

            /**
             * @brief Starts the server and begins accepting client connections.
             * @param p_port The port number to start the server on.
             * @throw std::runtime_error If initialization, socket creation, binding, or listening fails.
             */
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

            /**
             * @brief Runs the main server loop to accept and handle client connections and messages.
             */
            void run()
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
            }

            /**
             * @brief Accepts a new client connection and adds it to the client map.
             */
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

            /**
             * @brief Receives a message from a client.
             * @param p_clientId The ID of the client to receive the message from.
             * @param p_socket The socket of the client to receive the message from.
             * @return True if the message was successfully received, false otherwise.
             */
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
                            bytesRead = ::recv(p_socket, dataBuffer + totalBytesReceived, static_cast<int>(message->header().length()) - static_cast<int>(totalBytesReceived), 0);
                            if (bytesRead <= 0)
                            {
                                return false;
                            }
                            totalBytesReceived += static_cast<int>(bytesRead);
                        }
                    }
                    message->header().emitterID = p_clientId;
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

            /**
             * @brief Stops the server and cleans up resources.
             */
            void stop()
            {
                isRunning = false;
                closesocket(serverSocket);
                WSACleanup();
            }
        };

        std::unordered_map<ClientID, SOCKET> clients; ///< Map of connected clients and their sockets.
        std::mutex clientsMutex; ///< Mutex for thread-safe access to the client map.
        ClientID nextClientId = 10000; ///< The next client ID to assign.
        ConnectionCallback onConnectCallback; ///< Callback for client connections.
        DisconnectionCallback onDisconnectCallback; ///< Callback for client disconnections.
        std::unique_ptr<Acceptator> acceptor; ///< Unique pointer to the Acceptator object.
        MessagePool messagePool; ///< Pool of messages.
        spk::ThreadSafeDeque<MessageObject> messageQueue; ///< Thread-safe deque for message objects.

        /**
         * @brief Sends a message to a specific client.
         * @param p_clientID The ID of the client to send the message to.
         * @param p_message The message to send.
         */
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
                    sentBytes = ::send(clientSocket, reinterpret_cast<const char*>(p_message.data()), static_cast<int>(p_message.header().length()), 0);
                    if (sentBytes != static_cast<int>(p_message.header().length()))
                    {
                        std::cerr << "Failed to send message data." << std::endl;
                        return;
                    }
                }
            }
        }

    public:
        /**
         * @brief Constructs a Server object and sets up default connection and disconnection callbacks.
         */
        Server() :
            onConnectCallback([&](const ClientID& p_newlyConnectedClient){std::cout << "New connection detected: client ID [" << p_newlyConnectedClient << "]" << std::endl;}),
            onDisconnectCallback([&](const ClientID& p_disconnectedClient){std::cout << "Client ID [" << p_disconnectedClient << "] disconnected" << std::endl;}),
            acceptor(std::make_unique<Acceptator>(clients, clientsMutex, nextClientId, onConnectCallback, onDisconnectCallback, messagePool, messageQueue))
        {
        }

        /**
         * @brief Starts the server on a specified port.
         * @param p_serverPort The port number to start the server on.
         */
        void start(size_t p_serverPort)
        {
            acceptor->start(static_cast<int>(p_serverPort));
        }

        /**
         * @brief Stops the server.
         */
        void stop()
        {
            acceptor->stop();
        }

        /**
         * @brief Sends a message to a specific client.
         * @param p_clientID The ID of the client to send the message to.
         * @param p_message The message to send.
         */
        void sendTo(const ClientID& p_clientID, const spk::Message& p_message)
        {
            _internalSendTo(p_clientID, p_message);
        }

        /**
         * @brief Sends a message to a list of clients.
         * @param p_clients The list of client IDs to send the message to.
         * @param p_message The message to send.
         */
        void sendTo(const std::vector<ClientID>& p_clients, const spk::Message& p_message)
        {
            for (const auto& emitterID : p_clients)
            {
                _internalSendTo(emitterID, p_message);
            }
        }

        /**
         * @brief Sends a message to all connected clients.
         * @param p_message The message to send.
         */
        void sendToAll(const spk::Message& p_message)
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& pair : clients)
            {
                _internalSendTo(pair.first, p_message);
            }
        }

        /**
         * @brief Gets the thread-safe message queue.
         * @return Reference to the thread-safe message queue.
         */
        spk::ThreadSafeDeque<MessageObject>& messages()
        {
            return messageQueue;
        }
    };
}
