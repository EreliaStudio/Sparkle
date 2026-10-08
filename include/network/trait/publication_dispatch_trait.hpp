#pragma once
#include "publication_queue_trait.hpp"
namespace spk::Network
{
	// Schedules fair delivery; blocked or throwing sends retain their queued update.
	class PublicationDispatchTrait : protected PublicationQueueTrait
	{
	public:
		struct DispatchResult
		{
			std::size_t sent = 0, blocked = 0, errors = 0;
		};

	private:
		std::deque<PeerID> _roundRobin;
		Clock::duration _interval;
		Clock::time_point _nextPublication = Clock::time_point::min();
		bool _dispatchOne(PeerID peer, DispatchResult &result);

	protected:
		virtual void _capturePublicationChanges();
		[[nodiscard]] virtual bool _sendUpdate(PeerID, const Update<spk::Message> &, spk::Message::RequestID) = 0;
		void _openQueue(PeerID peer);
		void _closeQueue(PeerID peer);
		DispatchResult _dispatchPublication(Clock::time_point now, std::size_t maximumAttempts);

	public:
		explicit PublicationDispatchTrait(std::size_t maximumPending = 16384, Clock::duration interval = std::chrono::milliseconds(50));
		~PublicationDispatchTrait() override = default;
	};
}
