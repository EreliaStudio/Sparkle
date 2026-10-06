#pragma once

#include "exception.hpp"
#include "request.hpp"
#include "state_codec.hpp"
#include "update.hpp"

namespace spk::Network
{
	// One typed channel on an ordered transport. Uses native Sparkle encoding.
	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	class Protocol final
	{
	public:
		enum class Kind : std::uint8_t
		{
			Update,
			Request,
			Rejected
		};

	private:
		static constexpr std::uint32_t Magic = 0x32525053;
		spk::Message::Type _type;
		std::size_t _maximumBytes;
		[[nodiscard]] spk::Message::Writer _writer(Kind kind, spk::Message::RequestID requestID) const
		{
			spk::Message::Writer writer(_type);
			writer.setRequestID(requestID);
			writer << Magic << kind;
			return writer;
		}
		[[nodiscard]] spk::Message::Reader _reader(const spk::Message &message, Kind expected) const
		{
			if (kind(message) != expected)
			{
				throw spk::Exception("Unexpected replication message");
			}
			return message.reader(sizeof(Magic) + sizeof(Kind));
		}
		[[nodiscard]] spk::Message _finish(spk::Message::Writer writer) const
		{
			if (writer.size() > _maximumBytes)
			{
				throw spk::Exception("Replication frame too large");
			}
			return std::move(writer).build();
		}
		static void _end(const spk::Message::Reader &reader)
		{
			if (reader.readOffset() != reader.size())
			{
				throw spk::Exception("Trailing replication bytes");
			}
		}
		static void _validate(SessionID session, ObjectID object)
		{
			if (session.isNull() || object.isNull())
			{
				throw spk::Exception("Null network identity");
			}
		}
		static void _validate(const Update<State> &update)
		{
			_validate(update.session, update.object);
			if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy)
			{
				throw spk::Exception("Invalid update metadata");
			}
		}

	public:
		explicit Protocol(spk::Message::Type type, std::size_t maximumBytes = 2 * 1024 * 1024) :
			_type(type),
			_maximumBytes(maximumBytes)
		{
			if (maximumBytes < 64)
			{
				throw spk::Exception("Replication frame limit too small");
			}
		}
		[[nodiscard]] Kind kind(const spk::Message &message) const
		{
			if (message.type() != _type || message.size() > _maximumBytes)
			{
				throw spk::Exception("Invalid replication frame");
			}
			auto reader = message.reader();
			if (reader.get<std::uint32_t>() != Magic)
			{
				throw spk::Exception("Invalid replication protocol");
			}
			const auto result = reader.get<Kind>();
			if (result > Kind::Rejected)
			{
				throw spk::Exception("Invalid replication kind");
			}
			return result;
		}
		[[nodiscard]] spk::Message encode(const Update<State> &update, spk::Message::RequestID requestID = 0) const
		{
			_validate(update);
			if ((update.edit == Edit::Set) != static_cast<bool>(update.state))
			{
				throw spk::Exception("Invalid update state");
			}
			auto writer = _writer(Kind::Update, requestID);
			writer << update.session << update.object << update.tracking << update.revision << update.edit;
			if (update.state)
			{
				Codec::encode(writer, *update.state);
			}
			return _finish(std::move(writer));
		}
		[[nodiscard]] Update<State> decodeUpdate(const spk::Message &message) const
		{
			auto reader = _reader(message, Kind::Update);
			Update<State> update;
			reader >> update.session >> update.object >> update.tracking >> update.revision >> update.edit;
			_validate(update);
			if (update.edit == Edit::Set)
			{
				update.state = std::make_shared<const State>(Codec::decode(reader));
			}
			_end(reader);
			return update;
		}
		[[nodiscard]] spk::Message encode(const Request &request, Kind kind = Kind::Request) const
		{
			_validate(request.session, request.object);
			if (request.id == 0 || (kind != Kind::Request && kind != Kind::Rejected))
			{
				throw spk::Exception("Invalid acquisition request");
			}
			auto writer = _writer(kind, request.id);
			writer << request.session << request.object;
			return _finish(std::move(writer));
		}
		[[nodiscard]] Request decodeRequest(const spk::Message &message, Kind kind = Kind::Request) const
		{
			auto reader = _reader(message, kind);
			Request request;
			reader >> request.session >> request.object;
			request.id = message.requestID();
			_validate(request.session, request.object);
			if (request.id == 0 || (kind != Kind::Request && kind != Kind::Rejected))
			{
				throw spk::Exception("Invalid acquisition request");
			}
			_end(reader);
			return request;
		}
	};
}
