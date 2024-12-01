#include "structure/system/device/spk_controller.hpp"

std::ostream& operator << (std::ostream& p_os, const spk::Controller::Button& p_button)
{
	switch (p_button)
	{
	case spk::Controller::Button::A:
		p_os << "A"; break;
	case spk::Controller::Button::B:
		p_os << "B"; break;
	case spk::Controller::Button::X:
		p_os << "X"; break;
	case spk::Controller::Button::Y:
		p_os << "Y"; break;
	case spk::Controller::Button::LeftArrow:
		p_os << "LeftArrow"; break;
	case spk::Controller::Button::DownArrow:
		p_os << "DownArrow"; break;
	case spk::Controller::Button::RightArrow:
		p_os << "RightArrow"; break;
	case spk::Controller::Button::UpArrow:
		p_os << "UpArrow"; break;
	case spk::Controller::Button::Start:
		p_os << "Start"; break;
	case spk::Controller::Button::Select:
		p_os << "Select"; break;
	case spk::Controller::Button::R1:
		p_os << "R1"; break;
	case spk::Controller::Button::L1:
		p_os << "L1"; break;
	case spk::Controller::Button::R3:
		p_os << "R3"; break;
	case spk::Controller::Button::L3:
		p_os << "L3"; break;
	default:
		p_os << "Unknow"; break;
	}
	return (p_os);
}

std::ostream& operator << (std::ostream& p_os, const spk::Controller::Joystick::ID& p_joystickID)
{
	switch (p_joystickID)
	{
	case spk::Controller::Joystick::ID::Left:
		p_os << "Left"; break;
	case spk::Controller::Joystick::ID::Right:
		p_os << "Right"; break;
	}
	return (p_os);
}

std::ostream& operator << (std::ostream& p_os, const spk::Controller::Trigger::ID& p_triggerID)
{
	switch (p_triggerID)
	{
	case spk::Controller::Trigger::ID::Left:
		p_os << "Left"; break;
	case spk::Controller::Trigger::ID::Right:
		p_os << "Right"; break;
	}
	return (p_os);
}

std::wostream& operator << (std::wostream& p_os, const spk::Controller::Button& p_button)
{
	switch (p_button)
	{
	case spk::Controller::Button::A:
		p_os << "A"; break;
	case spk::Controller::Button::B:
		p_os << "B"; break;
	case spk::Controller::Button::X:
		p_os << "X"; break;
	case spk::Controller::Button::Y:
		p_os << "Y"; break;
	case spk::Controller::Button::LeftArrow:
		p_os << "LeftArrow"; break;
	case spk::Controller::Button::DownArrow:
		p_os << "DownArrow"; break;
	case spk::Controller::Button::RightArrow:
		p_os << "RightArrow"; break;
	case spk::Controller::Button::UpArrow:
		p_os << "UpArrow"; break;
	case spk::Controller::Button::Start:
		p_os << "Start"; break;
	case spk::Controller::Button::Select:
		p_os << "Select"; break;
	case spk::Controller::Button::R1:
		p_os << "R1"; break;
	case spk::Controller::Button::L1:
		p_os << "L1"; break;
	case spk::Controller::Button::R3:
		p_os << "R3"; break;
	case spk::Controller::Button::L3:
		p_os << "L3"; break;
	default:
		p_os << "Unknow"; break;
	}
	return (p_os);
}

std::wostream& operator << (std::wostream& p_os, const spk::Controller::Joystick::ID& p_joystickID)
{
	switch (p_joystickID)
	{
	case spk::Controller::Joystick::ID::Left:
		p_os << L"Left"; break;
	case spk::Controller::Joystick::ID::Right:
		p_os << L"Right"; break;
	}
	return (p_os);
}

std::wostream& operator << (std::wostream& p_os, const spk::Controller::Trigger::ID& p_triggerID)
{
	switch (p_triggerID)
	{
	case spk::Controller::Trigger::ID::Left:
		p_os << L"Left"; break;
	case spk::Controller::Trigger::ID::Right:
		p_os << L"Right"; break;
	}
	return (p_os);
}