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
		using ClientID = Message::Header::ClientID;
		using ContractProvider = TContractProvider<ClientID>;
		using Contract = Server::ContractProvider::Contract;
		using ConnectionCallback = Server::ContractProvider::Job;
		using DisconnectionCallback = Server::ContractProvider::Job;
		using MessagePool = spk::Pool<spk::Message>;
		using MessageObject = MessagePool::Object;

	private:
		class Acceptator
		{
		private:
			SOCKET serverSocket;
			std::unordered_map<ClientID, SOCKET> &clients;
			std::mutex &clientsMutex;
			ClientID &nextClientId;
			bool isRunning;
			Server::ContractProvider onConnectProvider;
			Server::ContractProvider onDisconnectProvider;
			MessagePool &messagePool;
			spk::ThreadSafeQueue<MessageObject> &messageQueue;

			void run();
			void acceptNewClient();
			bool receiveFromClient(ClientID p_clientId, SOCKET p_socket);

		public:
			std::thread receptionThread;

			Acceptator(std::unordered_map<ClientID, SOCKET> &clientsMap,
					   std::mutex &mutex,
					   ClientID &p_clientId,
					   MessagePool &pool,
					   spk::ThreadSafeQueue<MessageObject> &queue);
			~Acceptator();

			Server::Contract addOnConnectionCallback(const ConnectionCallback &p_connectionCallback);
			Server::Contract addOnDisconnectionCallback(const DisconnectionCallback &p_disconnectionCallback);

			void start(int p_port);
			void stop();
		};

		std::unordered_map<ClientID, SOCKET> clients;
		std::mutex clientsMutex;
		ClientID nextClientId = 10000;
		std::unique_ptr<Acceptator> acceptor;
		MessagePool messagePool;
		spk::ThreadSafeQueue<MessageObject> messageQueue;

		void _internalSendTo(const ClientID &p_clientID, const Message &p_message);

	public:
		Server();

		Server::Contract addOnConnectionCallback(const Server::ConnectionCallback &p_connectionCallback);
		Server::Contract addOnDisconnectionCallback(const Server::DisconnectionCallback &p_disconnectionCallback);

		void start(size_t p_serverPort);
		void stop();
		void sendTo(const ClientID &p_clientID, const spk::Message &p_message);
		void sendTo(const std::vector<ClientID> &p_clients, const spk::Message &p_message);
		void sendToAll(const spk::Message &p_message);
		spk::ThreadSafeQueue<MessageObject> &messages();
	};
}
