#pragma once

#include <source_location>
#include <string_view>

namespace spk::NetworkInternal
{
	void reportCurrentException(std::string_view context, std::source_location location = std::source_location::current()) noexcept;
}
