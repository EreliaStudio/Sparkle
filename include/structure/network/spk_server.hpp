#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "structure/container/spk_pool.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/network/spk_message.hpp"
#include "structure/spk_iostream.hpp"

namespace spk
{
	class Server
	{
	public:
		using ClientId = Message::Header::ClientId;
		using ContractProvider = TContractProvider<ClientId>;
		using Contract = Server::ContractProvider::Contract;
		using ConnectionCallback = Server::ContractProvider::Job;
		using DisconnectionCallback = Server::ContractProvider::Job;
		using MessagePool = spk::Pool<spk::Message>;
		using MessageObject = MessagePool::Object;

	private:
		class Acceptator
		{
		private:
			SOCKET _serverSocket;
			std::unordered_map<ClientId, SOCKET> &_clients;
			std::mutex &_clientsMutex;
			ClientId &_nextClientId;
			bool _isRunning;
			Server::ContractProvider _onConnectProvider;
			Server::ContractProvider _onDisconnectProvider;
			MessagePool &_messagePool;
			spk::ThreadSafeQueue<MessageObject> &_messageQueue;

			void _run();
			void _acceptNewClient();
			bool _receiveFromClient(ClientId p_clientId, SOCKET p_socket);
			int _prepareReadFds(fd_set &p_readfds);
			int _waitForActivity(fd_set &p_readfds, int p_maxFd);
			void _handleClients(fd_set &p_readfds);

		public:
			std::thread receptionThread;

			Acceptator(std::unordered_map<ClientId, SOCKET> &p_clientsMap,
					   std::mutex &p_mutex,
					   ClientId &p_clientId,
					   MessagePool &p_pool,
					   spk::ThreadSafeQueue<MessageObject> &p_queue);
			~Acceptator();

			Server::Contract addOnConnectionCallback(const ConnectionCallback &p_connectionCallback);
			Server::Contract addOnDisconnectionCallback(const DisconnectionCallback &p_disconnectionCallback);

			void start(int p_port);
			void stop();
		};

		std::unordered_map<ClientId, SOCKET> _clients;
		std::mutex _clientsMutex;
		ClientId _nextClientId = 10000;
		std::unique_ptr<Acceptator> _acceptor;
		MessagePool _messagePool;
		spk::ThreadSafeQueue<MessageObject> _messageQueue;

		void _internalSendTo(const ClientId &p_clientID, const Message &p_message);

	public:
		Server();

		Server::Contract addOnConnectionCallback(const Server::ConnectionCallback &p_connectionCallback);
		Server::Contract addOnDisconnectionCallback(const Server::DisconnectionCallback &p_disconnectionCallback);

		void start(size_t p_serverPort);
		void stop();
		void sendTo(const ClientId &p_clientID, const spk::Message &p_message);
		void sendTo(const std::vector<ClientId> &p_clients, const spk::Message &p_message);
		void sendToAll(const spk::Message &p_message);
		spk::ThreadSafeQueue<MessageObject> &messages();
	};
}
