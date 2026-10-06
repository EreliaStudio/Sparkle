#pragma once

#include "reply.hpp"
#include "state_codec.hpp"
#include "update.hpp"
#include <cstddef>
#include <exception.hpp>
#include <utility>

namespace spk::Network
{
	// One message type per typed channel. Native Sparkle scalar encoding.
	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	class Protocol final
	{
	public:
		enum class Kind : std::uint8_t
		{
			Update,
			Request,
			Reply
		};

	private:
		static constexpr std::uint32_t Magic = 0x31525053;
		spk::Message::Type _type;
		std::size_t _maximumBytes;
		[[nodiscard]] spk::Message::Writer _writer(Kind kind) const;
		[[nodiscard]] spk::Message::Reader _reader(const spk::Message &message, Kind kind) const;
		[[nodiscard]] spk::Message _finish(spk::Message::Writer writer) const;
		static void _end(const spk::Message::Reader &reader);
		static void _validate(const Request &request);
		static void _write(spk::Message::Writer &writer, const Request &request);
		[[nodiscard]] static Request _read(const spk::Message::Reader &reader);

	public:
		explicit Protocol(spk::Message::Type type, std::size_t maximumBytes = 2 * 1024 * 1024);
		[[nodiscard]] Kind kind(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Update<State> &update) const;
		[[nodiscard]] Update<State> decodeUpdate(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Request &request) const;
		[[nodiscard]] Request decodeRequest(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Reply &reply) const;
		[[nodiscard]] Reply decodeReply(const spk::Message &message) const;
	};
} // namespace spk::Network

#include "protocol.tpp"
