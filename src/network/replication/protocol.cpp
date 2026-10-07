#include "network/replication/protocol.hpp"
#include "exception.hpp"
#include <limits>

namespace spk::Network
{
	spk::Message::Writer Protocol::_writer(Kind kind, spk::Message::RequestID requestID) const
	{
		spk::Message::Writer writer(_type);
		writer.setRequestID(requestID);
		writer << Magic << kind;
		return writer;
	}
	spk::Message::Reader Protocol::_reader(const spk::Message &message, Kind expected) const
	{
		if (kind(message) != expected)
		{
			throw spk::Exception("Unexpected replication message");
		}
		return message.reader(sizeof(Magic) + sizeof(Kind));
	}
	spk::Message Protocol::_finish(spk::Message::Writer writer) const
	{
		if (writer.size() > _maximumBytes)
		{
			throw spk::Exception("Replication frame too large");
		}
		return std::move(writer).build();
	}
	void Protocol::_end(const spk::Message::Reader &reader)
	{
		if (reader.readOffset() != reader.size())
		{
			throw spk::Exception("Trailing replication bytes");
		}
	}
	void Protocol::_validate(SessionID session, ObjectID object)
	{
		if (session.isNull() || object.isNull())
		{
			throw spk::Exception("Null network identity");
		}
	}
	void Protocol::_validate(const Update<spk::Message> &update)
	{
		_validate(update.session, update.object);
		if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy)
		{
			throw spk::Exception("Invalid update metadata");
		}
	}
	Protocol::Protocol(spk::Message::Type type, std::size_t maximumBytes) :
		_type(type),
		_maximumBytes(maximumBytes)
	{
		if (maximumBytes < 64)
		{
			throw spk::Exception("Replication frame limit too small");
		}
	}
	spk::Message::Type Protocol::type() const noexcept
	{
		return _type;
	}
	spk::Message Protocol::encodeHandshake(SessionID token, SessionID session) const
	{
		if (token.isNull())
		{
			throw spk::Exception("Null handshake token");
		}
		auto writer = _writer(session.isNull() ? Kind::Hello : Kind::Session, 0);
		writer << token << session;
		return _finish(std::move(writer));
	}
	Protocol::Handshake Protocol::decodeHandshake(const spk::Message &message) const
	{
		const auto value = kind(message);
		if (value != Kind::Hello && value != Kind::Session)
		{
			throw spk::Exception("Unexpected handshake kind");
		}
		auto reader = _reader(message, value);
		Handshake result;
		reader >> result.token >> result.session;
		if (result.token.isNull() || (value == Kind::Hello) != result.session.isNull() || message.requestID() != 0)
		{
			throw spk::Exception("Invalid handshake");
		}
		_end(reader);
		return result;
	}
	Protocol::Kind Protocol::kind(const spk::Message &message) const
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
		if (result > Kind::Session)
		{
			throw spk::Exception("Invalid replication kind");
		}
		return result;
	}
	spk::Message Protocol::encode(const Update<spk::Message> &update, spk::Message::RequestID requestID) const
	{
		_validate(update);
		if ((update.edit == Edit::Set) != static_cast<bool>(update.payload))
		{
			throw spk::Exception("Invalid update state");
		}
		auto writer = _writer(Kind::Update, requestID);
		writer << update.session << update.object << update.tracking << update.revision << update.edit;
		if (update.payload)
		{
			if (update.payload->size() > std::numeric_limits<std::uint32_t>::max() ||
				update.payload->size() > _maximumBytes - writer.size() - sizeof(std::uint32_t))
			{
				throw spk::Exception("Replication payload too large");
			}
			writer << static_cast<std::uint32_t>(update.payload->size());
			writer.append(update.payload->data().data(), update.payload->size());
		}
		return _finish(std::move(writer));
	}
	Update<spk::Message> Protocol::decodeUpdate(const spk::Message &message) const
	{
		auto reader = _reader(message, Kind::Update);
		Update<spk::Message> update;
		reader >> update.session >> update.object >> update.tracking >> update.revision >> update.edit;
		_validate(update);
		if (update.edit == Edit::Set)
		{
			const auto length = reader.get<std::uint32_t>();
			if (length != reader.size() - reader.readOffset())
			{
				throw spk::Exception("Invalid replication payload length");
			}
			spk::Message::Writer payload;
			payload.append(reader.data().data() + reader.readOffset(), length);
			reader.skip(length);
			update.payload = std::move(payload).build();
		}
		_end(reader);
		return update;
	}
	spk::Message Protocol::encode(const Request &request, Kind kind) const
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
	Request Protocol::decodeRequest(const spk::Message &message, Kind kind) const
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
}
