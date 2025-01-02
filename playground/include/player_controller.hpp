#pragma once

#include "structure/engine/spk_component.hpp"

#include "event.hpp"

#include "structure/system/time/spk_timer.hpp"
#include "structure/math/spk_vector3.hpp"

class PlayerController : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;

	EventCenter::Type::Contract _upMotionContract;
	EventCenter::Type::Contract _leftMotionContract;
	EventCenter::Type::Contract _downMotionContract;
	EventCenter::Type::Contract _rightMotionContract;
	EventCenter::Type::Contract _idleMotionContract;

	spk::Timer _motionTimer;
	spk::Vector3 _direction;
	spk::Vector3 _origin; // Express in unit per millisecond
	spk::Vector3 _destination; // Express in unit per millisecond

	void movePlayer(const spk::Vector3& p_direction);
	void stopPlayer();

public:
	PlayerController(const std::wstring& p_name);

	void onUpdateEvent(spk::UpdateEvent& p_event) override;
};