#pragma once

#include "request.hpp"

#include "exception.hpp"
#include "update.hpp"
#include <concepts>

namespace spk::Network
{
	// Ordered replication envelope with typed serialization and captured-payload operations.
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
		static void _validate(const Update<spk::Message> &update);

	public:
		explicit Protocol(spk::Message::Type type, std::size_t maximumBytes = 2 * 1024 * 1024);
		[[nodiscard]] spk::Message::Type type() const noexcept;
		[[nodiscard]] spk::Message encodeHandshake(SessionID token, SessionID session = {}) const;
		[[nodiscard]] Handshake decodeHandshake(const spk::Message &message) const;

		[[nodiscard]] Kind kind(const spk::Message &message) const;
		[[nodiscard]] spk::Message encode(const Update<spk::Message> &update, spk::Message::RequestID requestID = 0) const;
		[[nodiscard]] Update<spk::Message> decodeUpdate(const spk::Message &message) const;
		// Typed operations use the same envelope as captured object payloads.
		template <typename State>
			requires requires(spk::Message::Writer &writer, const State &state) { writer << state; }
		[[nodiscard]] spk::Message encode(const Update<State> &update, spk::Message::RequestID requestID = 0) const
		{
			Update<spk::Message> captured{update.session, update.object, update.tracking, update.revision, update.edit, std::nullopt};
			_validate(captured);
			if ((update.edit == Edit::Set) != update.payload.has_value())
			{
				throw spk::Exception("Invalid update state");
			}
			if (update.payload)
			{
				spk::Message::Writer writer;
				writer << *update.payload;
				captured.payload = std::move(writer).build();
			}
			return encode(captured, requestID);
		}

		template <typename State>
			requires std::default_initializable<State> && std::move_constructible<State> &&
					 requires(const spk::Message::Reader &reader, State &state) { reader >> state; }
		[[nodiscard]] Update<State> decodeUpdate(const spk::Message &message) const
		{
			const auto captured = decodeUpdate(message);
			Update<State> update{captured.session, captured.object, captured.tracking, captured.revision, captured.edit, std::nullopt};
			if (captured.payload)
			{
				auto reader = captured.payload->reader();
				reader >> update.payload.emplace();
				_end(reader);
			}
			return update;
		}

		[[nodiscard]] spk::Message encode(const Request &request, Kind kind = Kind::Request) const;
		[[nodiscard]] Request decodeRequest(const spk::Message &message, Kind kind = Kind::Request) const;
	};
}
