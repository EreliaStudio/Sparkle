#pragma once
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
#include "replica_application_trait.hpp"
#include "replica_history_trait.hpp"
namespace spk::Network
{
	// Typed replication, independent of message encoding and transport.
	template <typename State>
	class ReplicaTrait : protected ReplicaHistoryTrait, protected ReplicaApplicationTrait<State>
	{
		SessionID _session;
		bool _active = false;
		void _clearReplicas()
		{
			for (const auto id : _activeReplicas())
			{
				this->_removeReplica(id);
			}
			_clearHistory();
			_onReplicasCleared();
		}

	protected:
		[[nodiscard]] SessionID _replicaSession() const
		{
			return _session;
		}
		[[nodiscard]] bool &_replicaOperation()
		{
			return _active;
		}
		void _requireReplicaIdle() const
		{
			if (_active)
			{
				throw spk::Exception("Replica mutation during application hook");
			}
		}
		virtual void _onReplicasCleared()
		{
		}
		virtual void _onReplicaRemoved(ObjectID)
		{
		}
		using ReplicaHistoryTrait::_tracksActive;
		[[nodiscard]] bool _acceptUpdate(const Update<State> &update)
		{
			if (_session.isNull() || update.session != _session)
			{
				return false;
			}
			if ((update.edit == Edit::Set) != static_cast<bool>(update.state))
			{
				throw spk::Exception("Invalid replica update state");
			}
			return _applyTracked(update.object, update.tracking, update.revision, update.edit, [&] {
				this->_applyReplica(update);
				if (update.edit != Edit::Set)
				{
					_onReplicaRemoved(update.object);
				}
			});
		}

	public:
		explicit ReplicaTrait(std::size_t maximumTracked = 65536) :
			ReplicaHistoryTrait(maximumTracked)
		{
		}
		virtual ~ReplicaTrait() = default;
		ReplicaTrait(const ReplicaTrait &) = delete;
		ReplicaTrait &operator=(const ReplicaTrait &) = delete;
		[[nodiscard]] bool receiveUpdate(const Update<State> &update)
		{
			OperationGuard guard(_active);
			return _acceptUpdate(update);
		}
		void resetSession(SessionID session)
		{
			OperationGuard guard(_active);
			if (session.isNull())
			{
				throw spk::Exception("Null network session");
			}
			if (session != _session)
			{
				_clearReplicas();
				_session = session;
			}
		}
		void closeSession()
		{
			OperationGuard guard(_active);
			_clearReplicas();
			_session = {};
		}
	};
}
