#pragma once
#include "exception.hpp"
#include "network/message.hpp"
#include "network/replication/operation_guard.hpp"
namespace spk::Network
{
	class ReplicableTrait
	{
		bool _reading = false;

	protected:
		// A throwing read may leave partial changes. Recovery is the application's responsibility.
		virtual void _readNetworkState(const spk::Message::Reader &reader) = 0;

	public:
		virtual ~ReplicableTrait() = default;
		virtual void readNetworkState(const spk::Message::Reader &reader) final;
	};
}
