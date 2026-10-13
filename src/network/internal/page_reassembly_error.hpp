#pragma once

#include "exception.hpp"

#include <cstdint>

namespace spk::NetworkInternal
{
	class PageReassemblyError final : public spk::Exception
	{
	public:
		enum class Reason
		{
			DuplicatePage,
			ConflictingPage,
			OverlappingPage,
			InvalidOffset,
			InvalidLength,
			InvalidFirstPage,
			InconsistentMetadata,
			Capacity
		};

	private:
		std::uint64_t _transferID;
		Reason _reason;

	public:
		PageReassemblyError(std::uint64_t transferID, Reason reason) :
			spk::Exception("Unable to reassemble network message: invalid page."),
			_transferID(transferID),
			_reason(reason)
		{
		}

		[[nodiscard]] std::uint64_t transferID() const noexcept
		{
			return _transferID;
		}

		[[nodiscard]] Reason reason() const noexcept
		{
			return _reason;
		}
	};
}
