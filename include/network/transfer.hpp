#pragma once

#include "design_pattern/contract_provider.hpp"

#include <cstdint>
#include <memory>

namespace spk::NetworkInternal
{
	class TransferCompletion;
}

namespace spk
{
	class Transfer
	{
	public:
		using ID = std::uint64_t;
		inline static constexpr ID InvalidID = 0;

		enum class Status
		{
			Pending,
			Success,
			Rejected,
			Failed,
			Indeterminate
		};

		enum class Reason
		{
			None,
			QueueOverflow,
			InvalidRecipient,
			ReceiverCapacity,
			DuplicatePage,
			ConflictingPage,
			OverlappingPage,
			InvalidOffset,
			InvalidLength,
			InvalidFirstPage,
			InconsistentMetadata,
			InactivityTimeout,
			AcknowledgmentTimeout,
			TransportFailure
		};

		using CompletionProvider = ContractProvider<Status, Reason>;
		using CompletionCallback = CompletionProvider::callback_type;
		using CompletionContract = CompletionProvider::Contract;

	private:
		struct Data;
		std::shared_ptr<Data> _data;

		friend class NetworkInternal::TransferCompletion;

		void _assignID(ID id) noexcept;
		[[nodiscard]] bool _complete(Status status, Reason reason) noexcept;

	public:
		Transfer();
		Transfer(const Transfer &) = default;
		Transfer(Transfer &&) noexcept = default;
		Transfer &operator=(const Transfer &) = default;
		Transfer &operator=(Transfer &&) noexcept = default;

		[[nodiscard]] bool operator==(const Transfer &other) const noexcept;
		[[nodiscard]] ID id() const noexcept;
		[[nodiscard]] Status status() const noexcept;
		[[nodiscard]] Reason reason() const noexcept;
		[[nodiscard]] bool isComplete() const noexcept;
		[[nodiscard]] CompletionContract subscribeToCompletion(CompletionCallback callback);
	};
}
