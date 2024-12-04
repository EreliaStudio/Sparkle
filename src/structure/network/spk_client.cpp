#include "structure/network/spk_client.hpp"

#include "spk_debug_macro.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void Client::_receive()
	{
		auto recv_exact = [](SOCKET p_s, char *p_buf, int p_total) -> int
		{
			int received = 0;
			while (received < p_total)
			{
				int r = ::recv(p_s, p_buf + received, p_total - received, 0);
				if (r == 0) // peer closed
				{
					return 0;
				}
				if (r < 0) // error
				{
					return r;
				}
				received += r;
			}
			return received;
		};

		while (_isConnected)
		{
			spk::Message::Header localHeader{};
			{
				const int headerSize = static_cast<int>(sizeof(localHeader));
				int r = recv_exact(_connectSocket, reinterpret_cast<char *>(&localHeader), headerSize);
				if (r <= 0)
				{
					disconnect();
					break;
				}
			}

			std::vector<std::byte> tmpPayload;
			if (localHeader.length > 0)
			{
				tmpPayload.resize(localHeader.length);
				int r = recv_exact(_connectSocket, reinterpret_cast<char *>(tmpPayload.data()), static_cast<int>(tmpPayload.size()));
				if (r <= 0)
				{
					disconnect();
					break;
				}
			}

			MessageObject message = _messagePool.obtain();
			message->_header = localHeader;

			if (localHeader.length > 0)
			{
				message->resize(localHeader.length);
				std::memcpy(message->_buffer.data(), tmpPayload.data(), tmpPayload.size());
			}

			_messageQueue.push(std::move(message));
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
