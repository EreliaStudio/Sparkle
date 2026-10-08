#pragma once
#include "publication_cadence_trait.hpp"
#include "publication_delivery_trait.hpp"
namespace spk::Network
{
	// Schedules fair delivery; blocked or throwing sends retain their queued update.
	class PublicationDispatchTrait : protected PublicationDeliveryTrait, protected PublicationCadenceTrait
	{
	public:
		using PublicationDeliveryTrait::DispatchResult;

	private:
		std::deque<PeerID> _roundRobin;
		DispatchResult _dispatchReady(std::size_t maximumAttempts);

	protected:
		virtual void _capturePublicationChanges();
		void _openQueue(PeerID peer);
		void _closeQueue(PeerID peer);
		DispatchResult _dispatchPublication(Clock::time_point now, std::size_t maximumAttempts);

	public:
		explicit PublicationDispatchTrait(std::size_t maximumPending = 16384, Clock::duration interval = std::chrono::milliseconds(50));
		~PublicationDispatchTrait() override = default;
	};
}
