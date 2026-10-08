#pragma once
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
#include "replica_application_trait.hpp"
#include "replica_history_trait.hpp"
namespace spk::Network
{
	// Opaque payload replication, independent of application serialization and transport.
	class ReplicaTrait : protected ReplicaHistoryTrait, protected ReplicaApplicationTrait
	{
		SessionID _session;
		bool _active = false;
		void _clearReplicas();

	protected:
		[[nodiscard]] SessionID _replicaSession() const;
		[[nodiscard]] bool &_replicaOperation();
		void _requireReplicaIdle() const;
		virtual void _onReplicasCleared();
		virtual void _onReplicaRemoved(ObjectID);
		using ReplicaHistoryTrait::_tracksActive;
		[[nodiscard]] bool _acceptUpdate(const Update<spk::Message> &update);

	public:
		explicit ReplicaTrait(std::size_t maximumTracked = 65536);
		virtual ~ReplicaTrait() = default;
		ReplicaTrait(const ReplicaTrait &) = delete;
		ReplicaTrait &operator=(const ReplicaTrait &) = delete;
		[[nodiscard]] bool receiveUpdate(const Update<spk::Message> &update);
		void resetSession(SessionID session);
		void closeSession();
	};
}
