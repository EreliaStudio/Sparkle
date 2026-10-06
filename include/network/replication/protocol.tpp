#pragma once

namespace spk::Network
{
	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message::Writer Protocol<State, Codec>::_writer(Kind kind) const
	{
		spk::Message::Writer writer(_type);
		writer << Magic << kind;
		return writer;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message::Reader Protocol<State, Codec>::_reader(const spk::Message &message, Kind kind) const
	{
		if (message.type() != _type || message.size() > _maximumBytes)
		{
			throw spk::Exception("Invalid replication frame");
		}
		auto reader = message.reader();
		if (reader.get<std::uint32_t>() != Magic || reader.get<Kind>() != kind)
		{
			throw spk::Exception("Invalid replication protocol/kind");
		}
		return reader;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message Protocol<State, Codec>::_finish(spk::Message::Writer writer) const
	{
		if (writer.size() > _maximumBytes)
		{
			throw spk::Exception("Replication frame too large");
		}
		return std::move(writer).build();
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	void Protocol<State, Codec>::_end(const spk::Message::Reader &reader)
	{
		if (reader.readOffset() != reader.size())
		{
			throw spk::Exception("Trailing replication bytes");
		}
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	void Protocol<State, Codec>::_validate(const Request &request)
	{
		if (request.session.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (request.object.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (request.attempt == 0)
		{
			throw spk::Exception("Invalid request attempt");
		}
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	void Protocol<State, Codec>::_write(spk::Message::Writer &writer, const Request &request)
	{
		_validate(request);
		writer << request.session << request.object << request.attempt;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	Request Protocol<State, Codec>::_read(const spk::Message::Reader &reader)
	{
		Request request;
		reader >> request.session >> request.object >> request.attempt;
		_validate(request);
		return request;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	Protocol<State, Codec>::Protocol(spk::Message::Type type, std::size_t maximumBytes) :
		_type(type),
		_maximumBytes(maximumBytes)
	{
		if (maximumBytes < 64)
		{
			throw spk::Exception("Replication frame limit too small");
		}
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	typename Protocol<State, Codec>::Kind Protocol<State, Codec>::kind(const spk::Message &message) const
	{
		if (message.type() != _type || message.size() > _maximumBytes)
		{
			throw spk::Exception("Invalid replication frame");
		}
		auto reader = message.reader();
		if (reader.get<std::uint32_t>() != Magic)
		{
			throw spk::Exception("Invalid replication magic");
		}
		const auto result = reader.get<Kind>();
		if (result > Kind::Reply)
		{
			throw spk::Exception("Invalid replication kind");
		}
		return result;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message Protocol<State, Codec>::encode(const Update<State> &update) const
	{
		if (update.session.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (update.object.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy ||
			(update.edit == Edit::Set) != static_cast<bool>(update.state))
		{
			throw spk::Exception("Invalid update");
		}
		auto writer = _writer(Kind::Update);
		writer << update.session << update.object << update.tracking << update.revision
			   << update.edit;
		if (update.state)
		{
			Codec::encode(writer, *update.state);
		}
		return _finish(std::move(writer));
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	Update<State> Protocol<State, Codec>::decodeUpdate(const spk::Message &message) const
	{
		auto reader = _reader(message, Kind::Update);
		Update<State> update;
		reader >> update.session >> update.object >> update.tracking >> update.revision >>
			update.edit;
		if (update.session.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (update.object.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy)
		{
			throw spk::Exception("Invalid update metadata");
		}
		if (update.edit == Edit::Set)
		{
			update.state = std::make_shared<const State>(Codec::decode(reader));
		}
		_end(reader);
		return update;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message Protocol<State, Codec>::encode(const Request &request) const
	{
		auto writer = _writer(Kind::Request);
		_write(writer, request);
		return _finish(std::move(writer));
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	Request Protocol<State, Codec>::decodeRequest(const spk::Message &message) const
	{
		auto reader = _reader(message, Kind::Request);
		auto request = _read(reader);
		_end(reader);
		return request;
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	spk::Message Protocol<State, Codec>::encode(const Reply &reply) const
	{
		if (reply.result > Reply::Result::Rejected)
		{
			throw spk::Exception("Invalid reply");
		}
		auto writer = _writer(Kind::Reply);
		_write(writer, reply.request);
		writer << reply.result;
		return _finish(std::move(writer));
	}

	template <typename State, typename Codec>
		requires StateCodec<Codec, State>
	Reply Protocol<State, Codec>::decodeReply(const spk::Message &message) const
	{
		auto reader = _reader(message, Kind::Reply);
		Reply reply{_read(reader)};
		reader >> reply.result;
		if (reply.result > Reply::Result::Rejected)
		{
			throw spk::Exception("Invalid reply");
		}
		_end(reader);
		return reply;
	}
}
