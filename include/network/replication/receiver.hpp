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
		explicit Receiver(std::size_t maximumTracked = 65536) :
			_maximumTracked(maximumTracked)
		{
			if (maximumTracked == 0)
			{
				throw spk::Exception("Invalid receiver capacity");
			}
		}
		// Explicit trusted handshake only; an ordinary update cannot change the session.
		// Caller must also clear its application replicas on a real session transition.
		void reset(SessionID session)
		{
			if (session.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (_session == session)
			{
				return;
			}
			_session = session;
			_tracking.clear();
		}
		[[nodiscard]] std::optional<Update<State>> receive(Update<State> update)
		{
			if (_session.isNull() || update.session != _session)
			{
				return std::nullopt;
			}
			if (update.object.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (update.tracking == 0 || update.revision == 0 || update.edit > Edit::Destroy ||
				(update.edit == Edit::Set) != static_cast<bool>(update.state))
			{
				throw spk::Exception("Invalid replication update");
			}
			if (!_tracking.contains(update.object) && _tracking.size() == _maximumTracked)
			{
				throw spk::Exception("Tracking history full; negotiate a fresh session");
			}
			auto &tracked = _tracking[update.object];
			if (update.tracking < tracked.identity)
			{
				return std::nullopt;
			}
			if (update.tracking == tracked.identity &&
				(!tracked.active ||
				 (update.edit == Edit::Set && update.revision <= tracked.revision)))
			{
				return std::nullopt;
			}
			tracked = {update.tracking, update.revision, update.edit == Edit::Set};
			return update;
		}
	};
} // namespace spk::Network
