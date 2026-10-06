#pragma once

namespace spk::Network
{
	template <typename State>
	Receiver<State>::Receiver(std::size_t maximumTracked) :
		_maximumTracked(maximumTracked)
	{
		if (maximumTracked == 0)
		{
			throw spk::Exception("Invalid receiver capacity");
		}
	}

	template <typename State>
	void Receiver<State>::reset(SessionID session)
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

	template <typename State>
	std::optional<Update<State>> Receiver<State>::receive(Update<State> update)
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
}
