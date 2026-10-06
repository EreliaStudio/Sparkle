#pragma once

#include <concepts>
#include <network/message.hpp>

namespace spk::Network
{
	// Validate semantic bounds and length fields before allocating.
	template <typename Codec, typename State>
	concept StateCodec = requires(spk::Message::Writer &writer, const spk::Message::Reader &reader, const State &state) {
		{ Codec::encode(writer, state) } -> std::same_as<void>;
		{ Codec::decode(reader) } -> std::same_as<State>;
	};
} // namespace spk::Network
