#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "network/spk_message.hpp"
#include "data_structure/spk_pool.hpp"
#include "data_structure/spk_thread_safe_deque.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace spk
{
    /**
     * @class Client
     * @brief Represents a network client that can connect to a server, send and receive messages.
     * 
     * This class provides the functionality to create a client that can connect to a server,
     * send messages to the server, and receive messages from the server. It uses Winsock for networking
     * and supports thread-safe operations.
     */
    class Client
    {
    public:
        using MessagePool = spk::Pool<spk::Message>; ///< Type alias for a pool of messages.
        using MessageObject = MessagePool::Object; ///< Type alias for a message object from the pool.
        using ConnectionCallback = std::function<void()>; ///< Callback type for client connection.
        using DisconnectionCallback = std::function<void()>; ///< Callback type for client disconnection.

    private:
        SOCKET _connectSocket; ///< The socket for connecting to the server.
        std::thread _receptionThread; ///< Thread for handling message reception.
        bool _isConnected; ///< Flag indicating if the client is connected.
        MessagePool _messagePool; ///< Pool of messages.
        spk::ThreadSafeDeque<MessageObject> _messageQueue; ///< Thread-safe deque for message objects.
        ConnectionCallback _onConnectCallback; ///< Callback for client connection.
        DisconnectionCallback _onDisconnectCallback; ///< Callback for client disconnection.

        /**
         * @brief Receives messages from the server.
         * 
         * This function runs in a separate thread and continuously listens for messages from the server.
         * It obtains message objects from the pool, reads the message headers and data, and pushes the
         * messages onto the message queue.
         */
        void _receive()
        {
            while (_isConnected)
            {
                MessageObject message = _messagePool.obtain();
                int headerSize = sizeof(spk::Message::Header);
                int bytesRead = recv(_connectSocket, reinterpret_cast<char*>(&message->header()), headerSize, 0);
                if (bytesRead == headerSize)
                {
                    if (message->header().length() > 0)
                    {
                        message->resize(message->header().length());
                        char* dataBuffer = reinterpret_cast<char*>(message->buffer().data());
                        size_t totalBytesReceived = 0;
                        while (totalBytesReceived < message->header().length())
                        {
                            bytesRead = recv(_connectSocket, dataBuffer + totalBytesReceived, static_cast<int>(message->header().length()) - static_cast<int>(totalBytesReceived), 0);
                            if (bytesRead <= 0)
                            {
                                break;
                            }
                            totalBytesReceived += bytesRead;
                        }
                    }
                    _messageQueue.push_back(std::move(message));
                }
                else if (bytesRead <= 0)
                {
                    disconnect();
                    break;
                }
            }
        }

    public:
        /**
         * @brief Constructs a Client object and sets up default connection and disconnection callbacks.
         */
        Client() :
            _connectSocket(INVALID_SOCKET),
            _isConnected(false),
            _messageQueue(),
            _onConnectCallback([&](){std::cout << "Connected to the server" << std::endl;}),
            _onDisconnectCallback([&](){std::cout << "Disconnected from the server" << std::endl;})
        {
        }

        /**
         * @brief Destructor for Client. Disconnects the client if connected.
         */
        ~Client()
        {
            disconnect();
        }

        /**
         * @brief Checks if the client is connected to the server.
         * @return True if the client is connected, false otherwise.
         */
        bool isConnected() const
        {
            return (_isConnected);
        }

        /**
         * @brief Connects the client to a server at a specified address and port.
         * @param p_address The address of the server to connect to.
         * @param p_port The port number of the server to connect to.
         * @throw std::runtime_error If Winsock initialization, socket creation, or connection fails.
         */
        void connect(const std::string& p_address, size_t p_port)
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                throw std::runtime_error("Failed to initialize Winsock.");
            }

            _connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (_connectSocket == INVALID_SOCKET)
            {
                WSACleanup();
                throw std::runtime_error("Cannot create socket.");
            }

            struct sockaddr_in serverAddress;
            serverAddress.sin_family = AF_INET;
            inet_pton(AF_INET, p_address.c_str(), &serverAddress.sin_addr);
            serverAddress.sin_port = htons(static_cast<u_short>(p_port));

            if (::connect(_connectSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
            {
                closesocket(_connectSocket);
                WSACleanup();
                throw std::runtime_error("Failed to connect to server.");
            }

            _isConnected = true;
            _onConnectCallback();
            std::thread(&Client::_receive, this).detach();
        }

        /**
         * @brief Disconnects the client from the server.
         */
        void disconnect()
        {
            if (_isConnected)
            {
                _isConnected = false;
                closesocket(_connectSocket);
                WSACleanup();
                _onDisconnectCallback();
            }
        }

        /**
         * @brief Sends a message to the server.
         * @param p_message The message to send.
         * @throw std::runtime_error If the client is not connected.
         */
        void send(const Message& p_message)
        {
            if (_isConnected)
            {
                int headerSize = sizeof(spk::Message::Header);
                int sentBytes = ::send(_connectSocket, reinterpret_cast<const char*>(&p_message.header()), headerSize, 0);

                if (sentBytes != headerSize)
                {
                    std::cerr << "Failed to send message header." << std::endl;
                    return;
                }

                if (p_message.header().length() > 0)
                {
                    sentBytes = ::send(_connectSocket, reinterpret_cast<const char*>(p_message.buffer().data()), static_cast<int>(p_message.header().length()), 0);
                    if (sentBytes != static_cast<int>(p_message.header().length()))
                    {
                        std::cerr << "Failed to send message data." << std::endl;
                        return;
                    }
                }
            }
            else
            {
                throw std::runtime_error("Can't send a message through a non-connected client.");
            }
        }

        /**
         * @brief Gets the thread-safe message queue.
         * @return Reference to the thread-safe message queue.
         */
        spk::ThreadSafeDeque<MessageObject>& messages()
        {
            return _messageQueue;
        }
    };
}
