#pragma once

#include <cstddef>

#ifndef SPK_PADDING
namespace spk
{
	template <std::size_t Size>
	struct Padding
	{
		static_assert(Size > 0, "spk::Padding must contain at least one float");
		float values[Size] = {};
	};
}

#	define SPK_DETAIL_CONCAT_IMPL(left, right) left##right
#	define SPK_DETAIL_CONCAT(left, right) SPK_DETAIL_CONCAT_IMPL(left, right)
#	define SPK_DETAIL_PADDING_NAME_IMPL(line, counter) SPK_DETAIL_CONCAT(SPK_DETAIL_CONCAT(SPK_DETAIL_CONCAT(_spk_padding_, line), _), counter)
#	define SPK_DETAIL_PADDING_NAME(line, counter) SPK_DETAIL_PADDING_NAME_IMPL(line, counter)
#	define SPK_PADDING(size) ::spk::Padding<size> SPK_DETAIL_PADDING_NAME(__LINE__, __COUNTER__)
#endif
