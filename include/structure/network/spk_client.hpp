#pragma once

#include "structure/container/spk_pool.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/network/spk_message.hpp"
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace spk
{
	class Client
	{
	public:
		using MessagePool = spk::Pool<spk::Message>;
		using MessageObject = MessagePool::Object;
		using ContractProvider = TContractProvider<>;
		using Contract = ContractProvider::Contract;
		using ConnectionCallback = ContractProvider::Job;
		using DisconnectionCallback = ContractProvider::Job;

	private:
		SOCKET _connectSocket;
		std::thread _receptionThread;
		bool _isConnected;
		MessagePool _messagePool;
		spk::ThreadSafeQueue<MessageObject> _messageQueue;
		ContractProvider _onConnectContractProvider;
		ContractProvider _onDisconnectContractProvider;

		void _receive();

	public:
		Client();
		~Client();

		Contract addOnConnectionCallback(const ConnectionCallback &p_connectionCallback);
		Contract addOnDisconnectionCallback(const DisconnectionCallback &p_disconnectionCallback);

		bool isConnected() const;
		void connect(const std::string &p_address, size_t p_port);
		void disconnect();
		void send(const Message &p_message);
		spk::ThreadSafeQueue<MessageObject> &messages();
	};
}
