#pragma once

#include "request.hpp"

#include "update.hpp"

namespace spk::Network
{
	// Replication envelope on an ordered transport. Application payloads remain opaque.
	class Protocol final
	{
	public:
		enum class Kind : std::uint8_t
		{
			Update,
			Request,
			Rejected,
			Hello,
			Session
		};

		struct Handshake
		{
			SessionID token, session;
		};

	private:
		static constexpr std::uint32_t Magic = 0x33525053;
		spk::Message::Type _type;
		std::size_t _maximumBytes;
		[[nodiscard]] spk::Message::Writer _writer(Kind kind, spk::Message::RequestID requestID) const;
		[[nodiscard]] spk::Message::Reader _reader(const spk::Message &message, Kind expected) const;
		[[nodiscard]] spk::Message _finish(spk::Message::Writer writer) const;
		static void _end(const spk::Message::Reader &reader);
		static void _validate(SessionID session, ObjectID object);
		static void _validate(const Update &update);

	public:
		explicit Protocol(spk::Message::Type type, std::size_t maximumBytes = 2 * 1024 * 1024);
		[[nodiscard]] spk::Message::Type type() const noexcept;
		[[nodiscard]] spk::Message encodeHandshake(SessionID token, SessionID session = {}) const;
		[[nodiscard]] Handshake decodeHandshake(const spk::Message &message) const;

		[[nodiscard]] Kind kind(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Update &update, spk::Message::RequestID requestID = 0) const;
		[[nodiscard]] Update decodeUpdate(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Request &request, Kind kind = Kind::Request) const;
		[[nodiscard]] Request decodeRequest(const spk::Message &message, Kind kind = Kind::Request) const;
	};
}
