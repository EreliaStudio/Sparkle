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
        SOCKET connectSocket;
        std::thread receptionThread;
        bool isConnected;
        MessagePool messagePool;
        spk::ThreadSafeDeque<MessageObject> messageQueue;
        ConnectionCallback onConnectCallback;
        DisconnectionCallback onDisconnectCallback;

    public:
        Client() :
            connectSocket(INVALID_SOCKET),
            isConnected(false),
            messageQueue(),
            onConnectCallback([&](){std::cout << "Connected to the server" << std::endl;}),
            onDisconnectCallback([&](){std::cout << "Disconnected from the server" << std::endl;})
        {
        }

        ~Client()
        {
            disconnect();
        }

        void connect(const std::string& address, size_t port)
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                throw std::runtime_error("Failed to initialize Winsock.");
            }

            connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (connectSocket == INVALID_SOCKET)
            {
                WSACleanup();
                throw std::runtime_error("Cannot create socket.");
            }

            struct sockaddr_in serverAddress;
            serverAddress.sin_family = AF_INET;
            inet_pton(AF_INET, address.c_str(), &serverAddress.sin_addr);
            serverAddress.sin_port = htons(static_cast<u_short>(port));

            if (::connect(connectSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
            {
                closesocket(connectSocket);
                WSACleanup();
                throw std::runtime_error("Failed to connect to server.");
            }

            isConnected = true;
            onConnectCallback();
            std::thread(&Client::receive, this).detach();
        }

        void disconnect()
        {
            if (isConnected)
            {
                isConnected = false;
                closesocket(connectSocket);
                WSACleanup();
                onDisconnectCallback();
            }
        }

        void send(const Message& message)
        {
            if (isConnected)
            {
                int headerSize = sizeof(spk::Message::Header);
                int sentBytes = ::send(connectSocket, reinterpret_cast<const char*>(&message.header()), headerSize, 0);
                if (sentBytes != headerSize)
                {
                    std::cerr << "Failed to send message header." << std::endl;
                    return;
                }

                if (message.header().length() > 0)
                {
                    sentBytes = ::send(connectSocket, reinterpret_cast<const char*>(message.buffer().data()), message.header().length(), 0);
                    if (sentBytes != static_cast<int>(message.header().length()))
                    {
                        std::cerr << "Failed to send message data." << std::endl;
                        return;
                    }
                }
            }
        }

        void receive()
        {
            while (isConnected)
            {
                MessageObject message = messagePool.obtain();
                int headerSize = sizeof(spk::Message::Header);
                int bytesRead = recv(connectSocket, reinterpret_cast<char*>(&message->header()), headerSize, 0);
                if (bytesRead == headerSize)
                {
                    if (message->header().length() > 0)
                    {
                        message->resize(message->header().length());
                        char* dataBuffer = reinterpret_cast<char*>(message->buffer().data());
                        size_t totalBytesReceived = 0;
                        while (totalBytesReceived < message->header().length())
                        {
                            bytesRead = recv(connectSocket, dataBuffer + totalBytesReceived, message->header().length() - totalBytesReceived, 0);
                            if (bytesRead <= 0)
                            {
                                break;
                            }
                            totalBytesReceived += bytesRead;
                        }
                    }
                    messageQueue.push_back(std::move(message));
                }
                else if (bytesRead <= 0)
                {
                    disconnect();
                    break;
                }
            }
        }

        spk::ThreadSafeDeque<MessageObject>& messages()
        {
            return messageQueue;
        }
    };
}
