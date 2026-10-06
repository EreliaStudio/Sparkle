#pragma once

#include "update.hpp"
#include <cstddef>
#include <exception.hpp>
#include <map>
#include <optional>

namespace spk::Network
{
	// No application object ownership; returns accepted updates for application.
	// All operations run on one owner thread.
	template <typename State>
	class Receiver final
	{
		struct Tracking
		{
			std::uint64_t identity = 0, revision = 0;
			bool active = false;
		};
		SessionID _session;
		std::size_t _maximumTracked;
		std::map<ObjectID, Tracking> _tracking;

	public:
		explicit Receiver(std::size_t maximumTracked = 65536);
		// Explicit trusted handshake only; an ordinary update cannot change the session.
		// Caller must also clear its application replicas on a real session transition.
		void reset(SessionID session);
		[[nodiscard]] std::optional<Update<State>> receive(Update<State> update);
	};
} // namespace spk::Network

#include "receiver.tpp"
