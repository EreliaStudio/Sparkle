#pragma once
#include "state.hpp"
#include <network/message.hpp>
namespace ReplicationTest
{
	struct Codec
	{
		inline static int decodes = 0;
		static void encode(spk::Message::Writer &writer, const State &state)
		{
			writer << state.value;
		}
		static State decode(const spk::Message::Reader &reader)
		{
			++decodes;
			return {reader.get<int>()};
		}
	};
} // namespace ReplicationTest
