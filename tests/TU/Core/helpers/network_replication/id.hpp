#pragma once
#include <type/uuid.hpp>
namespace ReplicationTest
{
	inline spk::UUID id(unsigned value)
	{
		spk::UUID::Storage bytes{};
		bytes[0] = static_cast<std::uint8_t>(value);
		return spk::UUID(bytes);
	}
} // namespace ReplicationTest
