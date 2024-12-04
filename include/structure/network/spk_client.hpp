#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "structure/network/spk_message.hpp"
#include "structure/container/spk_pool.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"

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
		spk::ThreadSafeQueue<MessageObject> _messageQueue;
		ConnectionCallback _onConnectCallback;
		DisconnectionCallback _onDisconnectCallback;

		void _receive();

	public:
		Client();
		~Client();

		bool isConnected() const;
		void connect(const std::string& p_address, size_t p_port);
		void disconnect();
		void send(const Message& p_message);
		spk::ThreadSafeQueue<MessageObject>& messages();
	};
}
