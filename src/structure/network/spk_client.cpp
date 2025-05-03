#include "structure/network/spk_client.hpp"

#include "spk_debug_macro.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void Client::_receive()
	{
		while (_isConnected)
		{
			MessageObject message = _messagePool.obtain();
			int headerSize = sizeof(spk::Message::Header);
			int bytesRead = recv(_connectSocket, reinterpret_cast<char *>(&message->_header), headerSize, 0);
			if (bytesRead == headerSize)
			{
				if (message->_header.length > 0)
				{
					message->resize(message->_header.length);
					char *dataBuffer = reinterpret_cast<char *>(message->_buffer.data());
					size_t totalBytesReceived = 0;
					while (totalBytesReceived < message->_header.length)
					{
						bytesRead = recv(_connectSocket,
										 dataBuffer + totalBytesReceived,
										 static_cast<int>(message->_header.length) - static_cast<int>(totalBytesReceived),
										 0);
						if (bytesRead <= 0)
						{
							break;
						}
						totalBytesReceived += bytesRead;
					}
				}
				_messageQueue.push(std::move(message));
			}
			else if (bytesRead <= 0)
			{
				disconnect();
				break;
			}
		}
	}

	Client::Client() :
		_connectSocket(INVALID_SOCKET),
		_isConnected(false),
		_messageQueue()
	{
	}

	Client::~Client()
	{
		disconnect();
	}

	Client::Contract Client::addOnConnectionCallback(const ConnectionCallback &p_connectionCallback)
	{
		return (_onConnectContractProvider.subscribe(p_connectionCallback));
	}

	Client::Contract Client::addOnDisconnectionCallback(const DisconnectionCallback &p_disconnectionCallback)
	{
		return (_onDisconnectContractProvider.subscribe(p_disconnectionCallback));
	}

	bool Client::isConnected() const
	{
		return _isConnected;
	}

	void Client::connect(const std::string &p_address, size_t p_port)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			GENERATE_ERROR("Failed to initialize Winsock.");
		}

		_connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_connectSocket == INVALID_SOCKET)
		{
			WSACleanup();
			GENERATE_ERROR("Cannot create socket.");
		}

		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		inet_pton(AF_INET, p_address.c_str(), &serverAddress.sin_addr);
		serverAddress.sin_port = htons(static_cast<u_short>(p_port));

		if (::connect(_connectSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
		{
			closesocket(_connectSocket);
			WSACleanup();
			GENERATE_ERROR("Failed to connect to server.");
		}

		_isConnected = true;
		_onConnectContractProvider.trigger();
		std::thread(&Client::_receive, this).detach();
	}

	void Client::disconnect()
	{
		if (_isConnected)
		{
			_isConnected = false;
			closesocket(_connectSocket);
			WSACleanup();
			_onDisconnectContractProvider.trigger();
		}
	}

	void Client::send(const Message &p_message)
	{
		if (_isConnected)
		{
			int headerSize = sizeof(spk::Message::Header);
			int sentBytes = ::send(_connectSocket, reinterpret_cast<const char *>(&p_message.header()), headerSize, 0);

			if (sentBytes != headerSize)
			{
				std::cerr << "Failed to send message header." << std::endl;
				return;
			}

			if (p_message.header().length > 0)
			{
				sentBytes =
					::send(_connectSocket, reinterpret_cast<const char *>(p_message.buffer().data()), static_cast<int>(p_message.header().length), 0);
				if (sentBytes != static_cast<int>(p_message.header().length))
				{
					std::cerr << "Failed to send message data." << std::endl;
					return;
				}
			}
		}
		else
		{
			GENERATE_ERROR("Can't send a message through a non-connected client.");
		}
	}

	spk::ThreadSafeQueue<Client::MessageObject> &Client::messages()
	{
		return _messageQueue;
	}
}
