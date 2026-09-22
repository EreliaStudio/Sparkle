#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace spk::NetworkInternal
{
	class Socket
	{
	public:
		struct ReceiveResult
		{
			std::size_t size = 0;
			bool disconnected = false;
		};

	private:
#ifdef _WIN32
		using NativeHandle = std::uintptr_t;
		static constexpr NativeHandle InvalidHandle = ~NativeHandle{0};
#else
		using NativeHandle = int;
		static constexpr NativeHandle InvalidHandle = -1;
#endif

		std::atomic<NativeHandle> _handle = InvalidHandle;

		explicit Socket(NativeHandle handle) noexcept;

		[[nodiscard]] NativeHandle _nativeHandle() const noexcept;

		friend std::vector<std::size_t> waitReadable(
			std::span<const Socket *const> sockets,
			std::chrono::milliseconds timeout);

	public:
		Socket() noexcept = default;
		~Socket();

		Socket(const Socket &) = delete;
		Socket &operator=(const Socket &) = delete;

		Socket(Socket &&other) noexcept;
		Socket &operator=(Socket &&other) noexcept;

		[[nodiscard]] static Socket connectTCP(std::string_view address, std::uint16_t port);
		[[nodiscard]] static Socket listenTCP(std::uint16_t port);

		[[nodiscard]] Socket accept() const;
		[[nodiscard]] std::uint16_t localPort() const;
		void sendAll(std::span<const std::byte> data) const;
		[[nodiscard]] ReceiveResult receive(std::span<std::byte> buffer) const;
		[[nodiscard]] bool receiveAll(std::span<std::byte> buffer) const;

		void close() noexcept;
		[[nodiscard]] bool valid() const noexcept;
	};

	[[nodiscard]] std::vector<std::size_t> waitReadable(
		std::span<const Socket *const> sockets,
		std::chrono::milliseconds timeout);
}
