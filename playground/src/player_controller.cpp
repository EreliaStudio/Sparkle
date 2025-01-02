#include "player_controller.hpp"

#include "structure/engine/spk_entity.hpp"

#include "chunk.hpp"

void PlayerController::movePlayer(const spk::Vector3& p_direction)
{
	_direction = p_direction;

	if (_motionTimer.state() == spk::Timer::State::Running)
	{
		return;
	}

	_origin = owner()->transform().localPosition();
	_destination = _origin + p_direction;
	_motionTimer.start();

}

void PlayerController::stopPlayer()
{
	_direction = 0;
}

PlayerController::PlayerController(const std::wstring& p_name) :
	spk::Component(p_name),
	_motionTimer(150LL, spk::TimeUnit::Millisecond),
	_upMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionUp, [&](){
		movePlayer(spk::Vector3(0, 1, 0));
	})),
	_leftMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionLeft, [&](){
		movePlayer(spk::Vector3(-1, 0, 0));
	})),
	_downMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionDown, [&](){
		movePlayer(spk::Vector3(0, -1, 0));
	})),
	_rightMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionRight, [&](){
		movePlayer(spk::Vector3(1, 0, 0));
	})),
	_idleMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionIdle, [&](){
		stopPlayer();
	}))
{

}

void PlayerController::onUpdateEvent(spk::UpdateEvent& p_event)
{
	if (p_event.deltaTime.milliseconds == 0)
	{
		return;
	}

	if (_motionTimer.state() != spk::Timer::State::Running)
	{
		if (_origin != _destination)
		{
			owner()->transform().place(_destination);

			if (Chunk::convertWorldToChunkPosition(_destination) != Chunk::convertWorldToChunkPosition(_origin))
			{
				EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
			}

			if (_direction == 0)
			{
				_origin = _destination;
			}
			else
			{
				movePlayer(_direction);
			}
			
		}
		return;
	} 

	if (_origin != _destination)
	{
		float ratio;
		if (_motionTimer.expectedDuration().value == 0)
			ratio = 1;
		else
			ratio = static_cast<double>(_motionTimer.elapsed().value) / static_cast<double>(_motionTimer.expectedDuration().value);
		owner()->transform().place(spk::Vector3::lerp(_origin, _destination, ratio));
	}
}