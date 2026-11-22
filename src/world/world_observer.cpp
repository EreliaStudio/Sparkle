#include "world/world_observer.hpp"

#include <utility>

#include "engine/entity3d.hpp"
#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"

namespace spk::World
{
	WorldObserver::WorldObserver(ChunkInterest interest) :
		Component3D("World observer"),
		_interest(std::move(interest))
	{
	}

	const ChunkInterest &WorldObserver::interest() const noexcept
	{
		return _interest;
	}

	void WorldObserver::setInterest(ChunkInterest interest)
	{
		if (_interest == interest)
		{
			return;
		}

		_interest = std::move(interest);
		_interestEditionProvider.trigger(*this);
	}

	Vector3 WorldObserver::worldPosition() const
	{
		const Entity3D *entity = owner();
		if (entity == nullptr)
		{
			throw spk::Exception("World observer is not attached to an Entity3D");
		}
		return entity->transform().position(ReferenceFrame::World);
	}

	WorldObserver::InterestEditionContract WorldObserver::subscribeToInterestEdition(InterestEditionCallback callback)
	{
		return _interestEditionProvider.subscribe(std::move(callback));
	}
}
