#pragma once
#include "network/message.hpp"
#include "network/replication/sequence.hpp"
#include "network/replication/types.hpp"
#include <map>
#include <optional>
namespace spk::Network
{
	// Immutable captured state survives detachment of its originating object.
	class PublicationSnapshotTrait
	{
	protected:
		struct Snapshot
		{
			std::uint64_t revision = 0;
			std::optional<spk::Message> payload;
		};

	private:
		std::map<ObjectID, Snapshot> _snapshots;
		Sequence _revisions;
		std::size_t _maximumObjects;

	protected:
		virtual void _beforeSnapshot(ObjectID);
		virtual void _onSnapshot(ObjectID);
		[[nodiscard]] bool _hasSnapshot(ObjectID id) const;
		[[nodiscard]] const Snapshot &_snapshot(ObjectID id) const;
		void _publish(ObjectID id, spk::Message payload);
		void _eraseSnapshot(ObjectID id);

	public:
		explicit PublicationSnapshotTrait(std::size_t maximumObjects = 16384);
		virtual ~PublicationSnapshotTrait() = default;
		PublicationSnapshotTrait(const PublicationSnapshotTrait &) = delete;
		PublicationSnapshotTrait &operator=(const PublicationSnapshotTrait &) = delete;
	};
}
