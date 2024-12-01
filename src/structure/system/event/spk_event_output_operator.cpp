#include "structure/system/event/spk_event.hpp"

std::ostream& operator << (std::ostream& p_os, const spk::MouseEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::MouseEvent::Type::Press:
		p_os << "Press"; break;
	case spk::MouseEvent::Type::Release:
		p_os << "Release"; break;
	case spk::MouseEvent::Type::DoubleClick:
		p_os << "DoubleClick"; break;
	case spk::MouseEvent::Type::Motion:
		p_os << "Motion"; break;
	case spk::MouseEvent::Type::Wheel:
		p_os << "Wheel"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::MouseEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::MouseEvent::Type::Press:
		p_os << L"Press"; break;
	case spk::MouseEvent::Type::Release:
		p_os << L"Release"; break;
	case spk::MouseEvent::Type::DoubleClick:
		p_os << L"DoubleClick"; break;
	case spk::MouseEvent::Type::Motion:
		p_os << L"Motion"; break;
	case spk::MouseEvent::Type::Wheel:
		p_os << L"Wheel"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}

std::ostream& operator << (std::ostream& p_os, const spk::KeyboardEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::KeyboardEvent::Type::Press:
		p_os << "Press"; break;
	case spk::KeyboardEvent::Type::Release:
		p_os << "Release"; break;
	case spk::KeyboardEvent::Type::Glyph:
		p_os << "Glyph"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::KeyboardEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::KeyboardEvent::Type::Press:
		p_os << L"Press"; break;
	case spk::KeyboardEvent::Type::Release:
		p_os << L"Release"; break;
	case spk::KeyboardEvent::Type::Glyph:
		p_os << L"Glyph"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}

std::ostream& operator << (std::ostream& p_os, const spk::ControllerEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::ControllerEvent::Type::Press:
		p_os << "Press"; break;
	case spk::ControllerEvent::Type::Release:
		p_os << "Release"; break;
	case spk::ControllerEvent::Type::JoystickMotion:
		p_os << "JoystickMotion"; break;
	case spk::ControllerEvent::Type::TriggerMotion:
		p_os << "TriggerMotion"; break;
	case spk::ControllerEvent::Type::DirectionalCrossMotion:
		p_os << "DirectionalCrossMotion"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::ControllerEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::ControllerEvent::Type::Press:
		p_os << L"Press"; break;
	case spk::ControllerEvent::Type::Release:
		p_os << L"Release"; break;
	case spk::ControllerEvent::Type::JoystickMotion:
		p_os << L"JoystickMotion"; break;
	case spk::ControllerEvent::Type::TriggerMotion:
		p_os << L"TriggerMotion"; break;
	case spk::ControllerEvent::Type::DirectionalCrossMotion:
		p_os << L"DirectionalCrossMotion"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}

std::ostream& operator << (std::ostream& p_os, const spk::SystemEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::SystemEvent::Type::Resize:
		p_os << "Resize"; break;
	case spk::SystemEvent::Type::TakeFocus:
		p_os << "TakeFocus"; break;
	case spk::SystemEvent::Type::LoseFocus:
		p_os << "LoseFocus"; break;
	case spk::SystemEvent::Type::Quit:
		p_os << "Quit"; break;
	case spk::SystemEvent::Type::Move:
		p_os << "Move"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::SystemEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::SystemEvent::Type::Resize:
		p_os << L"Resize"; break;
	case spk::SystemEvent::Type::TakeFocus:
		p_os << L"TakeFocus"; break;
	case spk::SystemEvent::Type::LoseFocus:
		p_os << L"LoseFocus"; break;
	case spk::SystemEvent::Type::Quit:
		p_os << L"Quit"; break;
	case spk::SystemEvent::Type::Move:
		p_os << L"Move"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}

std::ostream& operator << (std::ostream& p_os, const spk::PaintEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::PaintEvent::Type::Requested:
		p_os << "Requested"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::PaintEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::PaintEvent::Type::Requested:
		p_os << L"Requested"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}

std::ostream& operator << (std::ostream& p_os, const spk::UpdateEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::UpdateEvent::Type::Requested:
		p_os << "Requested"; break;
	default:
		p_os << "Unknown"; break;
	}
	return p_os;
}

std::wostream& operator << (std::wostream& p_os, const spk::UpdateEvent::Type& p_type)
{
	switch (p_type)
	{
	case spk::UpdateEvent::Type::Requested:
		p_os << L"Requested"; break;
	default:
		p_os << L"Unknown"; break;
	}
	return p_os;
}