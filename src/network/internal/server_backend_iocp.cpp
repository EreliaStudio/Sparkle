#include "network/internal/server_backend.hpp"

#ifdef _WIN32

#	include "exception.hpp"
#	include "network/internal/connection_id.hpp"
#	include <array>
#	include <cstddef>
#	include <cstdint>
#	include <memory>
#	include <string>
#	include <utility>
#	include <vector>
#	include <winsock2.h>
#	include <mswsock.h>
#	include <ws2tcpip.h>
#	include <Windows.h>

namespace spk::NetworkInternal
{
	namespace
	{
		using Frame = std::shared_ptr<std::vector<std::byte>>;
		constexpr ULONG_PTR WakeKey = 1;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t AcceptOperationCount = 4;

		class IocpServerBackend final : public ServerBackend
		{
		private:
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

			HANDLE _iocp = nullptr;
			LPFN_ACCEPTEX _acceptEx = nullptr;
			std::size_t _pendingOperations = 0;

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
					if (!_stopping)
					{
						_postAccept();
					}
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
					_postAccept();
					return;
				}

				auto session = _makeSession(std::move(operation->acceptedSocket));
				const ConnectionID id = session->id;
				_associate(session->socket);
				_sessions.emplace(id, session);
				_postRead(session);
				_notifyConnection(id);
				if (!_stopping)
				{
					_postAccept();
				}
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

				try
				{
					_appendReceived(*session, operation.buffer.data(), static_cast<std::size_t>(bytes));
				} catch (...)
				{
					_disconnect(session->id);
					return;
				}
				if (_sessions.contains(session->id) && !_stopping)
				{
					_postRead(session);
				}
			}

			void _postWrite(const std::shared_ptr<Session> &session)
			{
				if (session->writeInFlight || !_prepareWrite(*session))
				{
					return;
				}
				const Frame &frame = session->outgoing.front().bytes;
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
					session->writeInFlight = false;
					_disconnect(session->id);
					return;
				}

				_completeFrame(*session, static_cast<std::size_t>(bytes));
				if (!_sessions.contains(session->id))
				{
					return;
				}
				session->writeInFlight = false;
				_postWrite(session);
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

			void _run() noexcept override
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
						1000);
					try
					{
						if (overlapped != nullptr || success)
						{
							_handleCompletion(success, bytes, key, overlapped);
						}
						if (!_stopping)
						{
							_expireReassemblies();
						}
					} catch (...)
					{
						_beginStop();
					}
				}
			}

			void _openEvents() override
			{
				_pendingOperations = 0;
				_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
				if (_iocp == nullptr)
				{
					throw Exception("Unable to create IOCP [" + std::to_string(::GetLastError()) + "].");
				}
			}

			void _closeEvents() noexcept override
			{
				if (_iocp != nullptr)
				{
					::CloseHandle(_iocp);
					_iocp = nullptr;
				}
			}

			void _listen() override
			{
				_associate(_listener);
				_loadAcceptEx();
				for (std::size_t index = 0; index < AcceptOperationCount; ++index)
				{
					_postAccept();
				}
			}

			void _wake() noexcept override
			{
				if (_iocp != nullptr)
				{
					::PostQueuedCompletionStatus(_iocp, 0, WakeKey, nullptr);
				}
			}

			void _requestWrite(Session &session) override
			{
				_postWrite(_sessions.at(session.id));
			}

			void _writeSession(const std::shared_ptr<Session> &session) override
			{
				_postWrite(session);
			}

		public:
			explicit IocpServerBackend(ServerCallbacks callbacks) :
				ServerBackend(std::move(callbacks))
			{
			}

			~IocpServerBackend() override
			{
				stop();
			}
		};
	}

	std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks)
	{
		return std::make_unique<IocpServerBackend>(std::move(callbacks));
	}
}

#endif
