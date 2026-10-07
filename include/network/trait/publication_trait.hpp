#pragma once
#include "network/replication/operation_guard.hpp"
#include "object_interest_trait.hpp"
#include "peer_session_trait.hpp"
#include "publication_queue_trait.hpp"
#include "published_object_collection_trait.hpp"
namespace spk::Network
{
	class PublicationTrait : protected PublishedObjectCollectionTrait, protected PeerSessionTrait, protected ObjectInterestTrait, protected PublicationQueueTrait
	{
		using Objects = PublishedObjectCollectionTrait;
		using Queue = PublicationQueueTrait;
		bool _active = false;
		void _beforeSnapshot(ObjectID id) override;
		void _onSnapshot(ObjectID id) override;
		void _capturePublicationChanges() override;
		void _notify(ObjectID id, Edit edit);
		void _checkFollowers(ObjectID id) const;
		void _queueObject(PeerID peer, ObjectID id, Edit edit, spk::Message::RequestID requestID = 0);

	protected:
		void _requirePublicationIdle() const;
		[[nodiscard]] bool &_publicationOperation();
		using Objects::_hasSnapshot;
		using Objects::_publish;
		using PeerSessionTrait::_findSession;
		using PeerSessionTrait::_peerSession;
		using Queue::_room;
		virtual void _onPeerClosed(PeerID);
		virtual void _onObjectForgotten(PeerID, ObjectID);
		virtual void _onObjectDestroyed(ObjectID);
		void _follow(PeerID peer, ObjectID id, spk::Message::RequestID requestID = 0);

	public:
		struct Configuration
		{
			std::size_t maximumObjects = 16384, maximumPeers = 256;
			Clock::duration interval = std::chrono::milliseconds(50);
		};
		using DispatchResult = Queue::DispatchResult;
		PublicationTrait();
		explicit PublicationTrait(Configuration configuration);
		virtual ~PublicationTrait() = default;
		void registerObject(ObjectID id, PublishableTrait &instance);
		void unregisterObject(ObjectID id);
		void destroyObject(ObjectID id);
		[[nodiscard]] SessionID openPeer(PeerID peer);
		void closePeer(PeerID peer);
		void follow(PeerID peer, ObjectID id);
		void forget(PeerID peer, ObjectID id);
		DispatchResult dispatch(Clock::time_point now, std::size_t maximumAttempts = 64);
	};
}
