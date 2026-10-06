#pragma once

#include "request.hpp"
#include <cstdint>

namespace spk::Network
{
	struct Reply
	{
		Request request;
		enum class Result : std::uint8_t
		{
			Ready,
			Retry,
			Rejected
		} result = Result::Ready;
	};
} // namespace spk::Network
