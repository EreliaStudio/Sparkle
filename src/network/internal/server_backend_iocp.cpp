#include "network/internal/server_backend.hpp"

#ifdef _WIN32

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/socket.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mswsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

namespace spk::NetworkInternal
{
	namespace
	{
		using Frame = std::shared_ptr<std::vector<std::byte>>;
		constexpr ULONG_PTR WakeKey = 1;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t AcceptOperationCount = 4;

		struct Session
		{
			ConnectionID id;
			Socket socket;
			std::vector<std::byte> receivedBytes;
			std::deque<Frame> outgoing;
			std::size_t outgoingOffset = 0;
			bool writeInFlight = false;

			Session(ConnectionID id, Socket socket) :
				id(id),
				socket(std::move(socket))
			{
			}
		};

		enum class OperationKind
		{
			Accept,
			Read,
			Write
		};

		struct Operation
		{
			OVERLAPPED overlapped{};
			OperationKind kind;
			std::shared_ptr<Session> session;
			Socket acceptedSocket;
			std::array<std::byte, ReceptionBufferSize> buffer{};
			WSABUF wsabuf{};

			explicit Operation(OperationKind kind) :
				kind(kind)
			{
			}
		};

		enum class CommandKind
		{
			Send,
			Broadcast,
			Stop
		};

		struct Command
		{
			CommandKind kind;
			ConnectionID connection = InvalidConnectionID;
			Frame frame;
		};

		class IocpServerBackend final : public ServerBackend
		{
		private:
			ServerCallbacks _callbacks;
			Socket _listener;
			std::jthread _worker;
			std::atomic_bool _running = false;
			std::atomic_uint16_t _port = 0;
			HANDLE _iocp = nullptr;
			LPFN_ACCEPTEX _acceptEx = nullptr;
			ConnectionID _nextConnectionID = 1;
			std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;
			std::mutex _commandMutex;
			std::deque<Command> _commands;
			std::size_t _pendingOperations = 0;
			bool _stopping = false;

			void _notifyConnection(ConnectionID id) noexcept
			{
				try
				{
					_callbacks.onConnection(id);
				}
				catch (...)
				{
				}
			}

			void _notifyDisconnection(ConnectionID id) noexcept
			{
				try
				{
					_callbacks.onDisconnection(id);
				}
				catch (...)
				{
				}
			}

			void _publish(ReceivedMessage message) noexcept
			{
				try
				{
					_callbacks.onMessage(std::move(message));
				}
				catch (...)
				{
				}
			}

			void _associate(const Socket &socket)
			{
				const HANDLE result = ::CreateIoCompletionPort(
					reinterpret_cast<HANDLE>(static_cast<SOCKET>(socket.nativeHandle())),
					_iocp,
					0,
					0);
				if (result == nullptr)
				{
					throw Exception("Unable to associate socket with IOCP [" + std::to_string(::GetLastError()) + "].");
				}
			}

			void _loadAcceptEx()
			{
				GUID guid = WSAID_ACCEPTEX;
				DWORD bytes = 0;
				const int result = ::WSAIoctl(
					static_cast<SOCKET>(_listener.nativeHandle()),
					SIO_GET_EXTENSION_FUNCTION_POINTER,
					&guid,
					sizeof(guid),
					&_acceptEx,
					sizeof(_acceptEx),
					&bytes,
					nullptr,
					nullptr);
				if (result == SOCKET_ERROR || _acceptEx == nullptr)
				{
					throw Exception("Unable to load AcceptEx [" + std::to_string(::WSAGetLastError()) + "].");
				}
			}

			void _postAccept()
			{
				auto *operation = new Operation(OperationKind::Accept);
				operation->acceptedSocket = Socket::createTCPv4();
				DWORD bytes = 0;
				const DWORD addressSize = sizeof(sockaddr_storage) + 16;
				const BOOL result = _acceptEx(
					static_cast<SOCKET>(_listener.nativeHandle()),
					static_cast<SOCKET>(operation->acceptedSocket.nativeHandle()),
					operation->buffer.data(),
					0,
					addressSize,
					addressSize,
					&bytes,
					&operation->overlapped);
				if (!result && ::WSAGetLastError() != ERROR_IO_PENDING)
				{
					delete operation;
					throw Exception("Unable to post AcceptEx [" + std::to_string(::WSAGetLastError()) + "].");
				}
				++_pendingOperations;
			}

			void _completeAccept(std::unique_ptr<Operation> operation, bool success)
			{
				if (!success || _stopping)
				{
					return;
				}

				SOCKET listener = static_cast<SOCKET>(_listener.nativeHandle());
				const SOCKET accepted = static_cast<SOCKET>(operation->acceptedSocket.nativeHandle());
				if (::setsockopt(
						accepted,
						SOL_SOCKET,
						SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<const char *>(&listener),
						sizeof(listener)) != 0)
				{
					return;
				}

				const ConnectionID id = _nextConnectionID++;
				auto session = std::make_shared<Session>(id, std::move(operation->acceptedSocket));
				_associate(session->socket);
				_sessions.emplace(id, session);
				_postRead(session);
				_notifyConnection(id);
				_postAccept();
			}

			void _postRead(const std::shared_ptr<Session> &session)
			{
				auto *operation = new Operation(OperationKind::Read);
				operation->session = session;
				operation->wsabuf.buf = reinterpret_cast<char *>(operation->buffer.data());
				operation->wsabuf.len = static_cast<ULONG>(operation->buffer.size());
				DWORD flags = 0;
				DWORD bytes = 0;
				const int result = ::WSARecv(
					static_cast<SOCKET>(session->socket.nativeHandle()),
					&operation->wsabuf,
					1,
					&bytes,
					&flags,
					&operation->overlapped,
					nullptr);
				if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
				{
					delete operation;
					_disconnect(session->id);
					return;
				}
				++_pendingOperations;
			}

			void _completeRead(
				const std::shared_ptr<Session> &session,
				const Operation &operation,
				DWORD bytes,
				bool success)
			{
				if (!success || bytes == 0 || !_sessions.contains(session->id))
				{
					_disconnect(session->id);
					return;
				}

				session->receivedBytes.insert(
					session->receivedBytes.end(),
					operation.buffer.begin(),
					operation.buffer.begin() + static_cast<std::ptrdiff_t>(bytes));
				try
				{
					_extractMessages(*session);
				}
				catch (...)
				{
					_disconnect(session->id);
					return;
				}
				if (_sessions.contains(session->id) && !_stopping)
				{
					_postRead(session);
				}
			}

			void _extractMessages(Session &session)
			{
				std::size_t consumed = 0;
				while (_extractMessage(session, consumed))
				{
				}
				if (consumed != 0)
				{
					session.receivedBytes.erase(
						session.receivedBytes.begin(),
						session.receivedBytes.begin() + static_cast<std::ptrdiff_t>(consumed));
				}
			}

			[[nodiscard]] bool _extractMessage(Session &session, std::size_t &consumed)
			{
				const std::size_t remaining = session.receivedBytes.size() - consumed;
				if (remaining < FrameHeaderSize)
				{
					return false;
				}

				const auto *data = session.receivedBytes.data() + consumed;
				const auto header = decode(std::span<const std::byte, FrameHeaderSize>(data, FrameHeaderSize));
				const std::size_t frameSize = FrameHeaderSize + header.payloadSize;
				if (remaining < frameSize)
				{
					return false;
				}

				Message::Storage payload(header.payloadSize);
				std::copy_n(data + FrameHeaderSize, header.payloadSize, payload.begin());
				consumed += frameSize;
				_publish(ReceivedMessage{session.id, Message(header.type, std::move(payload))});
				return true;
			}

			void _queueFrame(const std::shared_ptr<Session> &session, Frame frame)
			{
				session->outgoing.push_back(std::move(frame));
				if (!session->writeInFlight)
				{
					_postWrite(session);
				}
			}

			void _postWrite(const std::shared_ptr<Session> &session)
			{
				if (session->outgoing.empty() || _stopping)
				{
					session->writeInFlight = false;
					return;
				}

				const Frame &frame = session->outgoing.front();
				auto *operation = new Operation(OperationKind::Write);
				operation->session = session;
				operation->wsabuf.buf = reinterpret_cast<char *>(
					frame->data() + session->outgoingOffset);
				operation->wsabuf.len = static_cast<ULONG>(
					frame->size() - session->outgoingOffset);
				DWORD bytes = 0;
				const int result = ::WSASend(
					static_cast<SOCKET>(session->socket.nativeHandle()),
					&operation->wsabuf,
					1,
					&bytes,
					0,
					&operation->overlapped,
					nullptr);
				if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
				{
					delete operation;
					_disconnect(session->id);
					return;
				}
				session->writeInFlight = true;
				++_pendingOperations;
			}

			void _completeWrite(
				const std::shared_ptr<Session> &session,
				DWORD bytes,
				bool success)
			{
				if (!success || bytes == 0 || !_sessions.contains(session->id))
				{
					_disconnect(session->id);
					return;
				}

				session->outgoingOffset += static_cast<std::size_t>(bytes);
				const Frame &frame = session->outgoing.front();
				if (session->outgoingOffset == frame->size())
				{
					session->outgoing.pop_front();
					session->outgoingOffset = 0;
				}
				session->writeInFlight = false;
				_postWrite(session);
			}

			void _disconnect(ConnectionID id) noexcept
			{
				const auto iterator = _sessions.find(id);
				if (iterator == _sessions.end())
				{
					return;
				}
				auto session = std::move(iterator->second);
				_sessions.erase(iterator);
				session->socket.close();
				_notifyDisconnection(id);
			}

			void _disconnectAll() noexcept
			{
				std::vector<ConnectionID> ids;
				ids.reserve(_sessions.size());
				for (const auto &entry : _sessions)
				{
					ids.push_back(entry.first);
				}
				for (const ConnectionID id : ids)
				{
					_disconnect(id);
				}
			}

			void _enqueue(Command command)
			{
				{
					const std::scoped_lock lock(_commandMutex);
					_commands.push_back(std::move(command));
				}
				::PostQueuedCompletionStatus(_iocp, 0, WakeKey, nullptr);
			}

			[[nodiscard]] std::deque<Command> _takeCommands()
			{
				const std::scoped_lock lock(_commandMutex);
				std::deque<Command> result;
				result.swap(_commands);
				return result;
			}

			void _processCommands()
			{
				for (Command &command : _takeCommands())
				{
					_processCommand(std::move(command));
					if (_stopping)
					{
						return;
					}
				}
			}

			void _processCommand(Command command)
			{
				if (command.kind == CommandKind::Stop)
				{
					_beginStop();
					return;
				}
				if (command.kind == CommandKind::Broadcast)
				{
					_broadcast(std::move(command.frame));
					return;
				}

				const auto iterator = _sessions.find(command.connection);
				if (iterator != _sessions.end())
				{
					_queueFrame(iterator->second, std::move(command.frame));
				}
			}

			void _broadcast(Frame frame)
			{
				std::vector<std::shared_ptr<Session>> sessions;
				sessions.reserve(_sessions.size());
				for (const auto &entry : _sessions)
				{
					sessions.push_back(entry.second);
				}
				for (const auto &session : sessions)
				{
					if (_sessions.contains(session->id))
					{
						_queueFrame(session, frame);
					}
				}
			}

			void _beginStop() noexcept
			{
				_stopping = true;
				_running = false;
				_listener.close();
				_disconnectAll();
			}

			void _handleCompletion(
				BOOL success,
				DWORD bytes,
				ULONG_PTR key,
				OVERLAPPED *overlapped)
			{
				if (overlapped == nullptr)
				{
					if (key == WakeKey)
					{
						_processCommands();
					}
					return;
				}

				std::unique_ptr<Operation> operation(
					reinterpret_cast<Operation *>(overlapped));
				if (_pendingOperations != 0)
				{
					--_pendingOperations;
				}

				switch (operation->kind)
				{
				case OperationKind::Accept:
					_completeAccept(std::move(operation), success != FALSE);
					break;
				case OperationKind::Read:
					_completeRead(operation->session, *operation, bytes, success != FALSE);
					break;
				case OperationKind::Write:
					_completeWrite(operation->session, bytes, success != FALSE);
					break;
				}
			}

			void _run() noexcept
			{
				while (!_stopping || _pendingOperations != 0)
				{
					DWORD bytes = 0;
					ULONG_PTR key = 0;
					OVERLAPPED *overlapped = nullptr;
					const BOOL success = ::GetQueuedCompletionStatus(
						_iocp,
						&bytes,
						&key,
						&overlapped,
						INFINITE);
					_handleCompletion(success, bytes, key, overlapped);
				}
			}

			void _openIocp()
			{
				_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
				if (_iocp == nullptr)
				{
					throw Exception("Unable to create IOCP [" + std::to_string(::GetLastError()) + "].");
				}
			}

			void _closeIocp() noexcept
			{
				if (_iocp != nullptr)
				{
					::CloseHandle(_iocp);
					_iocp = nullptr;
				}
			}

		public:
			explicit IocpServerBackend(ServerCallbacks callbacks) :
				_callbacks(std::move(callbacks))
			{
			}

			~IocpServerBackend() override
			{
				stop();
			}

			[[nodiscard]] bool isRunning() const noexcept override
			{
				return _running.load();
			}

			[[nodiscard]] std::uint16_t port() const noexcept override
			{
				return _port.load();
			}

			void start(std::uint16_t port) override
			{
				stop();
				_stopping = false;
				_pendingOperations = 0;
				_openIocp();
				try
				{
					_listener = Socket::listenTCP(port);
					_port = _listener.localPort();
					_associate(_listener);
					_loadAcceptEx();
					for (std::size_t index = 0; index < AcceptOperationCount; ++index)
					{
						_postAccept();
					}
				}
				catch (...)
				{
					_listener.close();
					_closeIocp();
					throw;
				}

				_running = true;
				_worker = std::jthread([this] {
					_run();
				});
			}

			void stop() override
			{
				if (_worker.joinable())
				{
					if (_running)
					{
						_enqueue(Command{CommandKind::Stop});
					}
					else if (_iocp != nullptr)
					{
						::PostQueuedCompletionStatus(_iocp, 0, WakeKey, nullptr);
					}
					if (_worker.get_id() != std::this_thread::get_id())
					{
						_worker.join();
					}
				}
				_listener.close();
				_disconnectAll();
				_closeIocp();
				_port = 0;
				_running = false;
				_stopping = true;
			}

			void sendTo(ConnectionID connection, const Message &message) override
			{
				if (!_running)
				{
					throw Exception("Unable to send through a stopped network server.");
				}
				_enqueue(Command{CommandKind::Send, connection, encode(message)});
			}

			void sendToAll(const Message &message) override
			{
				if (!_running)
				{
					throw Exception("Unable to send through a stopped network server.");
				}
				_enqueue(Command{CommandKind::Broadcast, InvalidConnectionID, encode(message)});
			}
		};
	}

	std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks)
	{
		return std::make_unique<IocpServerBackend>(std::move(callbacks));
	}
}

#endif
