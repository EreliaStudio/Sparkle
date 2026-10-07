#pragma once
#include "exception.hpp"
#include "network/replication/sequence.hpp"
#include "network/replication/types.hpp"
#include "publishable_trait.hpp"
#include <map>
#include <optional>
namespace spk::Network
{
	// Owner-thread observation; detachment retains the last immutable snapshot.
	class PublishedObjectCollectionTrait
	{
		struct RegisteredObject
		{
			PublishableTrait *instance = nullptr;
			std::weak_ptr<void> lifetime;
			spk::VersionedTrait::Version edition = 0;
			std::uint64_t revision = 0;
			std::optional<spk::Message> payload;
		};
		std::map<ObjectID, RegisteredObject> _objects;
		Sequence _revisions;
		std::size_t _maximumObjects;

	protected:
		virtual void _beforeSnapshot(ObjectID);
		virtual void _onSnapshot(ObjectID);
		[[nodiscard]] bool _hasSnapshot(ObjectID id) const;
		[[nodiscard]] const RegisteredObject &_snapshot(ObjectID id) const;
		void _publish(ObjectID id, spk::Message payload);
		void _captureChanges();
		void _registerObject(ObjectID id, PublishableTrait &instance);
		void _unregisterObject(ObjectID id);
		void _eraseSnapshot(ObjectID id);

	public:
		explicit PublishedObjectCollectionTrait(std::size_t maximumObjects = 16384);
		virtual ~PublishedObjectCollectionTrait() = default;
		PublishedObjectCollectionTrait(const PublishedObjectCollectionTrait &) = delete;
		PublishedObjectCollectionTrait &operator=(const PublishedObjectCollectionTrait &) = delete;
	};
}
