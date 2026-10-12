#include "network/internal/server_handshake.hpp"

#include "exception.hpp"

namespace spk::NetworkInternal
{
	ServerHandshake::ServerHandshake(std::chrono::milliseconds timeout) :
		_deadline(timeout)
	{
	}

	Handshake::Kind ServerHandshake::accept(std::span<const std::byte> frame)
	{
		if (_state != State::Pending)
		{
			throw Exception("Duplicate network handshake.");
		}
		const auto packet = Handshake::decode(frame);
		if (Handshake::validHello(packet))
		{
			_state = State::Accepting;
			return Handshake::Kind::Accepted;
		}
		_error = packet.validationError == Connection::Error::None ? Connection::Error::Rejected : packet.validationError;
		_state = State::Rejecting;
		return Handshake::Kind::Rejected;
	}

	bool ServerHandshake::written(Handshake::Kind kind)
	{
		if (kind == Handshake::Kind::Accepted && _state == State::Accepting)
		{
			_state = State::Ready;
			return true;
		}
		if (kind == Handshake::Kind::Rejected && _state == State::Rejecting)
		{
			_state = State::Rejected;
			return false;
		}
		throw Exception("Unexpected handshake write completion.");
	}

	ServerHandshake::State ServerHandshake::state() const noexcept
	{
		return _state;
	}

	bool ServerHandshake::ready() const noexcept
	{
		return _state == State::Ready;
	}

	bool ServerHandshake::expired() const noexcept
	{
		return !ready() && _deadline.expired();
	}

	Connection::Error ServerHandshake::error() const noexcept
	{
		return _error;
	}

	void ServerHandshake::fail(Connection::Error error) noexcept
	{
		_error = error;
	}
}
