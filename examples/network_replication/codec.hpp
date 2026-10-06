#pragma once
#include "state.hpp"
#include <network/message.hpp>
namespace SparkleNetworkExample
{
	struct Codec
	{
		static void encode(spk::Message::Writer &writer, const State &state)
		{
			writer << state.x << state.y;
		}
		[[nodiscard]] static State decode(const spk::Message::Reader &reader)
		{
			State state;
			reader >> state.x >> state.y;
			return state;
		}
	};
} // namespace SparkleNetworkExample
