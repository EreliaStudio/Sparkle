#pragma once
#include "network/replication/types.hpp"
namespace spk::Network
{
	// Controls publication timing without knowing peers, queues or transport.
	class PublicationCadenceTrait
	{
		Clock::duration _interval;
		Clock::time_point _nextPublication = Clock::time_point::min();

	protected:
		[[nodiscard]] bool _publicationDue(Clock::time_point now) const noexcept;
		void _advancePublication(Clock::time_point now);

	public:
		explicit PublicationCadenceTrait(Clock::duration interval = std::chrono::milliseconds(50));
		virtual ~PublicationCadenceTrait() = default;
	};
}
