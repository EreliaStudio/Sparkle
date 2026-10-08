#pragma once
#include "network/replication/edit.hpp"
#include "replica_revision_trait.hpp"
#include "network/replication/types.hpp"
#include <functional>
#include <map>
#include <vector>
namespace spk::Network
{
	class ReplicaHistoryTrait : protected ReplicaRevisionTrait
	{
		struct Tracking
		{
			std::uint64_t identity = 0, revision = 0;
			bool active = false;
		};
		std::map<ObjectID, Tracking> _tracking;
		std::size_t _maximumTracked;

	protected:
		[[nodiscard]] bool _applyTracked(ObjectID object, std::uint64_t identity, std::uint64_t revision, Edit edit, const std::function<void()> &apply);
		[[nodiscard]] bool _tracksActive(ObjectID object, std::uint64_t identity) const;
		[[nodiscard]] std::vector<ObjectID> _activeReplicas() const;
		void _clearHistory();

	public:
		explicit ReplicaHistoryTrait(std::size_t maximumTracked = 65536);
		ReplicaHistoryTrait(const ReplicaHistoryTrait &) = delete;
		ReplicaHistoryTrait &operator=(const ReplicaHistoryTrait &) = delete;
		virtual ~ReplicaHistoryTrait() = default;
	};
}
