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
    class Client
    {
    public:
        using MessagePool = spk::Pool<spk::Message>;
        using MessageObject = MessagePool::Object;
        using ConnectionCallback = std::function<void()>;
        using DisconnectionCallback = std::function<void()>;

    private:
        SOCKET _connectSocket;
        std::thread _receptionThread;
        bool _isConnected;
        MessagePool _messagePool;
        spk::ThreadSafeDeque<MessageObject> _messageQueue;
        ConnectionCallback _onConnectCallback;
        DisconnectionCallback _onDisconnectCallback;

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
                            bytesRead = recv(_connectSocket, dataBuffer + totalBytesReceived, message->header().length() - totalBytesReceived, 0);
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
        Client() :
            _connectSocket(INVALID_SOCKET),
            _isConnected(false),
            _messageQueue(),
            _onConnectCallback([&](){std::cout << "Connected to the server" << std::endl;}),
            _onDisconnectCallback([&](){std::cout << "Disconnected from the server" << std::endl;})
        {
        }

        ~Client()
        {
            disconnect();
        }

        bool isConnected() const
        {
            return (_isConnected);
        }

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
                    sentBytes = ::send(_connectSocket, reinterpret_cast<const char*>(p_message.buffer().data()), p_message.header().length(), 0);
                    if (sentBytes != static_cast<int>(p_message.header().length()))
                    {
                        std::cerr << "Failed to send message data." << std::endl;
                        return;
                    }
                }
            }
            else
            {
                spk::throwException("Can't send a message thought a non-connected client");
            }
        }

        spk::ThreadSafeDeque<MessageObject>& messages()
        {
            return _messageQueue;
        }
    };
}
