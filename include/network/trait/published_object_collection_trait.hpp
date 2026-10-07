#pragma once
#include "exception.hpp"
#include "network/replication/sequence.hpp"
#include "network/replication/types.hpp"
#include "publishable_trait.hpp"
#include <map>
namespace spk::Network
{
	// Owner-thread observation; detachment retains the last immutable snapshot.
	template <typename State>
	class PublishedObjectCollectionTrait
	{
		struct RegisteredObject
		{
			PublishableTrait<State> *instance = nullptr;
			std::weak_ptr<void> lifetime;
			spk::VersionedTrait::Version edition = 0;
			std::uint64_t revision = 0;
			std::shared_ptr<const State> state;
		};
		std::map<ObjectID, RegisteredObject> _objects;
		Sequence _revisions;
		std::size_t _maximumObjects;

	protected:
		virtual void _beforeSnapshot(ObjectID)
		{
		}
		virtual void _onSnapshot(ObjectID)
		{
		}
		[[nodiscard]] bool _hasSnapshot(ObjectID id) const
		{
			return _objects.contains(id);
		}
		[[nodiscard]] const RegisteredObject &_snapshot(ObjectID id) const
		{
			auto found = _objects.find(id);
			if (found == _objects.end())
			{
				throw spk::Exception("Unknown replication object");
			}
			return found->second;
		}
		void _publish(ObjectID id, State state)
		{
			if (id.isNull() || (!_objects.contains(id) && _objects.size() >= _maximumObjects))
			{
				throw spk::Exception("Invalid object identity or object limit reached");
			}
			_beforeSnapshot(id);
			auto snapshot = std::make_shared<const State>(std::move(state));
			const auto revision = _revisions.next();
			auto &object = _objects[id];
			object.state = std::move(snapshot);
			object.revision = revision;
			_onSnapshot(id);
		}
		void _captureChanges()
		{
			for (auto &[id, object] : _objects)
			{
				if (object.lifetime.expired() || object.edition == object.instance->version())
				{
					continue;
				}
				const auto edition = object.instance->version();
				_publish(id, object.instance->buildNetworkState());
				object.edition = edition;
			}
		}
		void _registerObject(ObjectID id, PublishableTrait<State> &instance)
		{
			auto found = _objects.find(id);
			if (found != _objects.end() && !found->second.lifetime.expired())
			{
				throw spk::Exception("Network object already registered");
			}
			const auto edition = instance.version();
			_publish(id, instance.buildNetworkState());
			auto &object = _objects.at(id);
			object.instance = &instance;
			object.lifetime = instance._lifetime;
			object.edition = edition;
		}
		void _unregisterObject(ObjectID id)
		{
			if (auto found = _objects.find(id); found != _objects.end())
			{
				found->second.lifetime.reset();
				found->second.instance = nullptr;
			}
		}
		void _eraseSnapshot(ObjectID id)
		{
			_objects.erase(id);
		}

	public:
		explicit PublishedObjectCollectionTrait(std::size_t maximumObjects = 16384) :
			_maximumObjects(maximumObjects)
		{
			if (maximumObjects == 0)
			{
				throw spk::Exception("Invalid object capacity");
			}
		}
		virtual ~PublishedObjectCollectionTrait() = default;
		PublishedObjectCollectionTrait(const PublishedObjectCollectionTrait &) = delete;
		PublishedObjectCollectionTrait &operator=(const PublishedObjectCollectionTrait &) = delete;
	};
}
