#include "network/internal/socket.hpp"

#include "exception.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <chrono>
#include <memory>
#include <stop_token>
#include <string>

#ifdef _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <arpa/inet.h>
#	include <fcntl.h>
#	include <netdb.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <unistd.h>
#endif

namespace spk::NetworkInternal
{
	namespace
	{
#ifdef _WIN32
		using SystemSocket = SOCKET;
		constexpr SystemSocket InvalidSystemSocket = INVALID_SOCKET;

		class SocketRuntime
		{
		public:
			SocketRuntime()
			{
				WSADATA data{};
				const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
				if (result != 0)
				{
					throw Exception("Unable to initialize WinSock [" + std::to_string(result) + "].");
				}
			}

			~SocketRuntime()
			{
				::WSACleanup();
			}
		};

		void ensureSocketRuntime()
		{
			static SocketRuntime runtime;
			(void)runtime;
		}

		[[nodiscard]] SystemSocket createSystemSocket(int family)
		{
			return ::WSASocketW(family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		}

		[[nodiscard]] int lastSocketError() noexcept
		{
			return ::WSAGetLastError();
		}

		void closeSystemSocket(SystemSocket socket) noexcept
		{
			::closesocket(socket);
		}

		[[nodiscard]] bool interrupted(int error) noexcept
		{
			return error == WSAEINTR;
		}
#else
		using SystemSocket = int;
		constexpr SystemSocket InvalidSystemSocket = -1;

		void ensureSocketRuntime()
		{
		}

		[[nodiscard]] SystemSocket createSystemSocket(int family)
		{
			return ::socket(family, SOCK_STREAM, IPPROTO_TCP);
		}

		[[nodiscard]] int lastSocketError() noexcept
		{
			return errno;
		}

		void closeSystemSocket(SystemSocket socket) noexcept
		{
			::close(socket);
		}

		[[nodiscard]] bool interrupted(int error) noexcept
		{
			return error == EINTR;
		}
#endif

		[[nodiscard]] SystemSocket checkedTCP(int family)
		{
			const SystemSocket socket = createSystemSocket(family);
			if (socket == InvalidSystemSocket)
			{
				throw Exception("Unable to create TCP socket [" + std::to_string(lastSocketError()) + "].");
			}
			return socket;
		}

		void shutdownSystemSocket(SystemSocket socket) noexcept
		{
#ifdef _WIN32
			::shutdown(socket, SD_BOTH);
#else
			::shutdown(socket, SHUT_RDWR);
#endif
		}

		[[nodiscard]] std::string portString(std::uint16_t port)
		{
			return std::to_string(static_cast<unsigned int>(port));
		}

		[[nodiscard]] bool connecting(int error) noexcept
		{
#ifdef _WIN32
			return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
			return error == EINPROGRESS || error == EWOULDBLOCK;
#endif
		}

		[[nodiscard]] bool waiting(int error) noexcept
		{
#ifdef _WIN32
			return error == WSAEWOULDBLOCK;
#else
			return error == EAGAIN || error == EWOULDBLOCK;
#endif
		}

		[[nodiscard]] bool waitForSocket(
			SystemSocket socket, bool writing, std::chrono::steady_clock::time_point deadline, std::stop_token stop)
		{
			while (!stop.stop_requested() && std::chrono::steady_clock::now() < deadline)
			{
				fd_set sockets;
				FD_ZERO(&sockets);
				FD_SET(socket, &sockets);
				const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
					deadline - std::chrono::steady_clock::now());
				const auto duration = std::clamp(remaining, std::chrono::milliseconds(1), std::chrono::milliseconds(50));
				timeval timeout{static_cast<long>(duration.count() / 1000), static_cast<long>((duration.count() % 1000) * 1000)};
				const int ready = ::select(static_cast<int>(socket) + 1,
					writing ? nullptr : &sockets, writing ? &sockets : nullptr, nullptr, &timeout);
				if (ready > 0)
				{
					return true;
				}
				if (ready < 0 && !interrupted(lastSocketError()))
				{
					throw Exception("Unable to poll TCP socket.");
				}
			}
			return false;
		}

		[[nodiscard]] bool connectReady(
			SystemSocket socket, const addrinfo &entry,
			std::chrono::steady_clock::time_point deadline, std::stop_token stop)
		{
			if (::connect(socket, entry.ai_addr, static_cast<int>(entry.ai_addrlen)) == 0)
			{
				return true;
			}
			if (!connecting(lastSocketError()) || !waitForSocket(socket, true, deadline, stop))
			{
				return false;
			}
			int error = 0;
#ifdef _WIN32
			int length = sizeof(error);
			return ::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &length) == 0 && error == 0;
#else
			socklen_t length = sizeof(error);
			return ::getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) == 0 && error == 0;
#endif
		}

	}

	Socket::Socket(NativeHandle handle) noexcept :
		_handle(handle)
	{
	}

	Socket::~Socket()
	{
		close();
	}

	Socket::Socket(Socket &&other) noexcept :
		_handle(other._handle.exchange(InvalidHandle))
	{
	}

	Socket &Socket::operator=(Socket &&other) noexcept
	{
		if (this != &other)
		{
			close();
			_handle.store(other._handle.exchange(InvalidHandle));
		}
		return *this;
	}

	Socket Socket::createTCPv4()
	{
		ensureSocketRuntime();
		return adopt(static_cast<NativeHandle>(checkedTCP(AF_INET)));
	}

	Socket Socket::adopt(NativeHandle handle) noexcept
	{
		return Socket(handle);
	}

	Socket Socket::connectTCP(std::string_view address, std::uint16_t port)
	{
		ensureSocketRuntime();

		addrinfo hints{};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		addrinfo *addresses = nullptr;
		const std::string host(address);
		const std::string service = portString(port);
		if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses) != 0)
		{
			throw Exception("Unable to resolve network address [" + host + "].");
		}

		for (addrinfo *entry = addresses; entry != nullptr; entry = entry->ai_next)
		{
			const SystemSocket candidate = createSystemSocket(entry->ai_family);
			if (candidate == InvalidSystemSocket)
			{
				continue;
			}
			if (::connect(candidate, entry->ai_addr, static_cast<int>(entry->ai_addrlen)) == 0)
			{
				::freeaddrinfo(addresses);
				return adopt(static_cast<NativeHandle>(candidate));
			}
			closeSystemSocket(candidate);
		}

		::freeaddrinfo(addresses);
		throw Exception("Unable to connect to [" + host + ":" + service + "].");
	}

	Socket Socket::connectTCP(
		std::string_view address, std::uint16_t port, std::chrono::steady_clock::time_point deadline, std::stop_token stop)
	{
		ensureSocketRuntime();
		addrinfo hints{};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		addrinfo *addresses = nullptr;
		const std::string host(address);
		if (::getaddrinfo(host.c_str(), portString(port).c_str(), &hints, &addresses) != 0)
		{
			throw Exception("Unable to resolve network address [" + host + "].");
		}
		std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> resolved(addresses, &::freeaddrinfo);
		for (auto *entry = addresses; entry != nullptr && !stop.stop_requested(); entry = entry->ai_next)
		{
			if (std::chrono::steady_clock::now() >= deadline)
			{
				break;
			}
			const auto native = createSystemSocket(entry->ai_family);
			if (native == InvalidSystemSocket)
			{
				continue;
			}
			Socket candidate = adopt(static_cast<NativeHandle>(native));
			candidate.setNonBlocking(true);
			if (connectReady(native, *entry, deadline, stop))
			{
				candidate.setNonBlocking(false);
				return candidate;
			}
		}
		throw Exception("Unable to connect to [" + host + ":" + portString(port) + "].");
	}

	Socket Socket::listenTCP(std::uint16_t port)
	{
		Socket result = createTCPv4();
		const SystemSocket handle = static_cast<SystemSocket>(result.nativeHandle());

		int reuse = 1;
#ifdef _WIN32
		::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));
#else
		::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_port = htons(port);

		if (::bind(handle, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0)
		{
			throw Exception("Unable to bind TCP socket [" + std::to_string(lastSocketError()) + "].");
		}
		if (::listen(handle, SOMAXCONN) != 0)
		{
			throw Exception("Unable to listen on TCP socket [" + std::to_string(lastSocketError()) + "].");
		}
		return result;
	}

	Socket Socket::accept() const
	{
		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle());
		const SystemSocket accepted = ::accept(handle, nullptr, nullptr);
		if (accepted == InvalidSystemSocket)
		{
			throw Exception("Unable to accept TCP connection [" + std::to_string(lastSocketError()) + "].");
		}
		return adopt(static_cast<NativeHandle>(accepted));
	}

	std::uint16_t Socket::localPort() const
	{
		sockaddr_in address{};
#ifdef _WIN32
		int size = sizeof(address);
#else
		socklen_t size = sizeof(address);
#endif
		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle());
		if (::getsockname(handle, reinterpret_cast<sockaddr *>(&address), &size) != 0)
		{
			throw Exception("Unable to query TCP socket port [" + std::to_string(lastSocketError()) + "].");
		}
		return ntohs(address.sin_port);
	}

	void Socket::setNonBlocking(bool value) const
	{
		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle());
#ifdef _WIN32
		u_long mode = value ? 1UL : 0UL;
		if (::ioctlsocket(handle, FIONBIO, &mode) != 0)
		{
			throw Exception("Unable to configure TCP socket blocking mode [" + std::to_string(lastSocketError()) + "].");
		}
#else
		const int flags = ::fcntl(handle, F_GETFL, 0);
		if (flags < 0)
		{
			throw Exception("Unable to query TCP socket flags [" + std::to_string(lastSocketError()) + "].");
		}
		const int updated = value ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
		if (::fcntl(handle, F_SETFL, updated) != 0)
		{
			throw Exception("Unable to configure TCP socket blocking mode [" + std::to_string(lastSocketError()) + "].");
		}
#endif
	}

	void Socket::sendAll(std::span<const std::byte> data) const
	{
		std::size_t offset = 0;
		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle());
		while (offset < data.size())
		{
			const int requested = static_cast<int>(
				std::min<std::size_t>(data.size() - offset, INT_MAX));
#ifdef _WIN32
			const int sent = ::send(
				handle,
				reinterpret_cast<const char *>(data.data() + offset),
				requested,
				0);
#else
			const int sent = static_cast<int>(::send(
				handle,
				data.data() + offset,
				static_cast<std::size_t>(requested),
				MSG_NOSIGNAL));
#endif
			if (sent > 0)
			{
				offset += static_cast<std::size_t>(sent);
				continue;
			}
			const int error = lastSocketError();
			if (sent < 0 && interrupted(error))
			{
				continue;
			}
			throw Exception("Unable to send TCP data [" + std::to_string(error) + "].");
		}
	}

	Socket::ReceiveResult Socket::receive(std::span<std::byte> buffer) const
	{
		if (buffer.empty())
		{
			return {};
		}

		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle());
		while (true)
		{
			const int requested = static_cast<int>(
				std::min<std::size_t>(buffer.size(), INT_MAX));
#ifdef _WIN32
			const int received = ::recv(
				handle,
				reinterpret_cast<char *>(buffer.data()),
				requested,
				0);
#else
			const int received = static_cast<int>(::recv(
				handle,
				buffer.data(),
				static_cast<std::size_t>(requested),
				0));
#endif
			if (received > 0)
			{
				return ReceiveResult{static_cast<std::size_t>(received), false};
			}
			if (received == 0)
			{
				return ReceiveResult{0, true};
			}
			const int error = lastSocketError();
			if (interrupted(error))
			{
				continue;
			}
			throw Exception("Unable to receive TCP data [" + std::to_string(error) + "].");
		}
	}

	bool Socket::receiveAll(std::span<std::byte> buffer) const
	{
		std::size_t offset = 0;
		while (offset < buffer.size())
		{
			const ReceiveResult result = receive(buffer.subspan(offset));
			if (result.disconnected)
			{
				return false;
			}
			offset += result.size;
		}
		return true;
	}

	bool Socket::receiveAllUntil(
		std::span<std::byte> buffer, std::chrono::steady_clock::time_point deadline, std::stop_token stop) const
	{
		setNonBlocking(true);
		std::size_t offset = 0;
		try
		{
			while (offset < buffer.size() && waitForSocket(static_cast<SystemSocket>(nativeHandle()), false, deadline, stop))
			{
				const auto socket = static_cast<SystemSocket>(nativeHandle());
#ifdef _WIN32
				const int received = ::recv(socket, reinterpret_cast<char *>(buffer.data() + offset),
					static_cast<int>(std::min<std::size_t>(buffer.size() - offset, INT_MAX)), 0);
#else
				const int received = static_cast<int>(::recv(socket, buffer.data() + offset, buffer.size() - offset, 0));
#endif
				if (received == 0)
				{
					break;
				}
				if (received > 0)
				{
					offset += static_cast<std::size_t>(received);
				}
				else if (!waiting(lastSocketError()) && !interrupted(lastSocketError()))
				{
					throw Exception("Unable to receive handshake data.");
				}
			}
		} catch (...)
		{
			if (valid())
			{
				setNonBlocking(false);
			}
			throw;
		}
		if (valid())
		{
			setNonBlocking(false);
		}
		return offset == buffer.size();
	}

	void Socket::close() noexcept
	{
		const NativeHandle nativeHandle = _handle.exchange(InvalidHandle);
		if (nativeHandle == InvalidHandle)
		{
			return;
		}

		const SystemSocket handle = static_cast<SystemSocket>(nativeHandle);
		shutdownSystemSocket(handle);
		closeSystemSocket(handle);
	}

	bool Socket::valid() const noexcept
	{
		return _handle.load() != InvalidHandle;
	}

	Socket::NativeHandle Socket::nativeHandle() const noexcept
	{
		return _handle.load();
	}
}
