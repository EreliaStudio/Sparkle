#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "structure/spk_iostream.hpp"
#include "structure/network/spk_message.hpp"
#include "structure/container/spk_pool.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace spk
{
	class Server
	{
	public:
		using ClientID = Message::Header::ClientID;
		using ConnectionCallback = std::function<void(const ClientID&)>;
		using DisconnectionCallback = std::function<void(const ClientID&)>;
		using MessagePool = spk::Pool<spk::Message>;
		using MessageObject = MessagePool::Object;

	private:
		class Acceptator
		{
		private:
			SOCKET serverSocket;
			std::unordered_map<ClientID, SOCKET>& clients;
			std::mutex& clientsMutex;
			ClientID& nextClientId;
			bool isRunning;
			ConnectionCallback& onConnectCallback;
			DisconnectionCallback& onDisconnectCallback;
			MessagePool& messagePool;
			spk::ThreadSafeQueue<MessageObject>& messageQueue;

			void run();
			void acceptNewClient();
			bool receiveFromClient(ClientID p_clientId, SOCKET p_socket);

		public:
			std::thread receptionThread;

			Acceptator(std::unordered_map<ClientID, SOCKET>& clientsMap, std::mutex& mutex, ClientID& p_clientId,
					   ConnectionCallback& connectCallback, DisconnectionCallback& disconnectCallback,
					   MessagePool& pool, spk::ThreadSafeQueue<MessageObject>& queue);
			~Acceptator();

			void start(int p_port);
			void stop();
		};

		std::unordered_map<ClientID, SOCKET> clients;
		std::mutex clientsMutex;
		ClientID nextClientId = 10000;
		ConnectionCallback onConnectCallback;
		DisconnectionCallback onDisconnectCallback;
		std::unique_ptr<Acceptator> acceptor;
		MessagePool messagePool;
		spk::ThreadSafeQueue<MessageObject> messageQueue;

		void _internalSendTo(const ClientID& p_clientID, const Message& p_message);

	public:
		Server();
		void start(size_t p_serverPort);
		void stop();
		void sendTo(const ClientID& p_clientID, const spk::Message& p_message);
		void sendTo(const std::vector<ClientID>& p_clients, const spk::Message& p_message);
		void sendToAll(const spk::Message& p_message);
		spk::ThreadSafeQueue<MessageObject>& messages();
	};
}
