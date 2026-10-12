#pragma once

#include "design_pattern/contract_provider.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace spk
{
	class Client;
	class Server;

	class Connection
	{
	public:
		using ID = std::uint64_t;
		inline static constexpr ID InvalidID = 0;

		enum class Status
		{
			Disconnected,
			Pending,
			Connected
		};

		enum class Error
		{
			None,
			Refused,
			Timeout,
			InvalidMagic,
			VersionMismatch,
			Rejected,
			TransportFailure
		};

		enum class OverflowPolicy
		{
			Reject,
			SkipRecipient,
			DisconnectRecipient
		};

		using StatusProvider = ContractProvider<Status, Error>;
		using StatusCallback = StatusProvider::callback_type;
		using StatusContract = StatusProvider::Contract;

	private:
		struct Data;
		std::shared_ptr<Data> _data;

		friend class Client;
		friend class Server;

		void _assignID(ID id) noexcept;
		void _setStatus(Status status, Error error = Error::None);

	public:
		Connection();
		Connection(const Connection &) = default;
		Connection(Connection &&) noexcept = default;
		Connection &operator=(const Connection &) = default;
		Connection &operator=(Connection &&) noexcept = default;

		[[nodiscard]] ID id() const noexcept;
		[[nodiscard]] Status status() const noexcept;
		[[nodiscard]] Error error() const noexcept;
		[[nodiscard]] bool isConnected() const noexcept;
		[[nodiscard]] std::size_t maxQueuedBytes() const noexcept;
		void setMaxQueuedBytes(std::size_t bytes) noexcept;
		[[nodiscard]] StatusContract subscribeToStatusChange(StatusCallback callback);
	};
}
