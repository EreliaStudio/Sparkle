#include "network/trait/publication_cadence_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	PublicationCadenceTrait::PublicationCadenceTrait(Clock::duration interval) :
		_interval(interval)
	{
		if (interval < Clock::duration::zero())
		{
			throw spk::Exception("Invalid publication interval");
		}
	}
	bool PublicationCadenceTrait::_publicationDue(Clock::time_point now) const noexcept
	{
		return now >= _nextPublication;
	}
	void PublicationCadenceTrait::_advancePublication(Clock::time_point now)
	{
		_nextPublication = now + _interval;
	}
}
