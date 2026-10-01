#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace spk::NetworkInternal
{
	class Socket
	{
	public:
		using NativeHandle = std::uintptr_t;

		struct ReceiveResult
		{
			std::size_t size = 0;
			bool disconnected = false;
		};

	private:
		static constexpr NativeHandle InvalidHandle = ~NativeHandle{0};

		std::atomic<NativeHandle> _handle = InvalidHandle;

		explicit Socket(NativeHandle handle) noexcept;

	public:
		Socket() noexcept = default;
		~Socket();

		Socket(const Socket &) = delete;
		Socket &operator=(const Socket &) = delete;

		Socket(Socket &&other) noexcept;
		Socket &operator=(Socket &&other) noexcept;

		[[nodiscard]] static Socket createTCPv4();
		[[nodiscard]] static Socket adopt(NativeHandle handle) noexcept;
		[[nodiscard]] static Socket connectTCP(std::string_view address, std::uint16_t port);
		[[nodiscard]] static Socket listenTCP(std::uint16_t port);

		[[nodiscard]] Socket accept() const;
		[[nodiscard]] std::uint16_t localPort() const;
		void setNonBlocking(bool value) const;
		void sendAll(std::span<const std::byte> data) const;
		[[nodiscard]] ReceiveResult receive(std::span<std::byte> buffer) const;
		[[nodiscard]] bool receiveAll(std::span<std::byte> buffer) const;

		void close() noexcept;
		[[nodiscard]] bool valid() const noexcept;
		[[nodiscard]] NativeHandle nativeHandle() const noexcept;
	};
}
