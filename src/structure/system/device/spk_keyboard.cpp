#include "structure/system/device/spk_keyboard.hpp"

std::ostream& operator << (std::ostream& p_os, const spk::Keyboard::Key& p_key)
{
	switch (p_key)
	{
	case spk::Keyboard::Key::Backspace:
		p_os << "Backspace"; break;
	case spk::Keyboard::Key::Tab:
		p_os << "Tab"; break;
	case spk::Keyboard::Key::Clear:
		p_os << "Clear"; break;
	case spk::Keyboard::Key::Return:
		p_os << "Return"; break;
	case spk::Keyboard::Key::Shift:
		p_os << "Shift"; break;
	case spk::Keyboard::Key::Control:
		p_os << "Control"; break;
	case spk::Keyboard::Key::Alt:
		p_os << "Alt"; break;
	case spk::Keyboard::Key::Pause:
		p_os << "Pause"; break;
	case spk::Keyboard::Key::Capslock:
		p_os << "Capslock"; break;
	case spk::Keyboard::Key::Escape:
		p_os << "Escape"; break;
	case spk::Keyboard::Key::Convert:
		p_os << "Convert"; break;
	case spk::Keyboard::Key::Non_convert:
		p_os << "Non_convert"; break;
	case spk::Keyboard::Key::Accept:
		p_os << "Accept"; break;
	case spk::Keyboard::Key::Mode_change:
		p_os << "Mode_change"; break;
	case spk::Keyboard::Key::Space:
		p_os << "Space"; break;
	case spk::Keyboard::Key::Prior:
		p_os << "Prior"; break;
	case spk::Keyboard::Key::Next:
		p_os << "Next"; break;
	case spk::Keyboard::Key::End:
		p_os << "End"; break;
	case spk::Keyboard::Key::Home:
		p_os << "Home"; break;
	case spk::Keyboard::Key::LeftArrow:
		p_os << "LeftArrow"; break;
	case spk::Keyboard::Key::UpArrow:
		p_os << "UpArrow"; break;
	case spk::Keyboard::Key::RightArrow:
		p_os << "RightArrow"; break;
	case spk::Keyboard::Key::DownArrow:
		p_os << "DownArrow"; break;
	case spk::Keyboard::Key::Select:
		p_os << "Select"; break;
	case spk::Keyboard::Key::Print:
		p_os << "Print"; break;
	case spk::Keyboard::Key::Execute:
		p_os << "Execute"; break;
	case spk::Keyboard::Key::Snapshot:
		p_os << "Snapshot"; break;
	case spk::Keyboard::Key::Insert:
		p_os << "Insert"; break;
	case spk::Keyboard::Key::Delete:
		p_os << "Delete"; break;
	case spk::Keyboard::Key::Help:
		p_os << "Help"; break;
	case spk::Keyboard::Key::Key0:
		p_os << "Key0"; break;
	case spk::Keyboard::Key::Key1:
		p_os << "Key1"; break;
	case spk::Keyboard::Key::Key2:
		p_os << "Key2"; break;
	case spk::Keyboard::Key::Key3:
		p_os << "Key3"; break;
	case spk::Keyboard::Key::Key4:
		p_os << "Key4"; break;
	case spk::Keyboard::Key::Key5:
		p_os << "Key5"; break;
	case spk::Keyboard::Key::Key6:
		p_os << "Key6"; break;
	case spk::Keyboard::Key::Key7:
		p_os << "Key7"; break;
	case spk::Keyboard::Key::Key8:
		p_os << "Key8"; break;
	case spk::Keyboard::Key::Key9:
		p_os << "Key9"; break;
	case spk::Keyboard::Key::A:
		p_os << "A"; break;
	case spk::Keyboard::Key::B:
		p_os << "B"; break;
	case spk::Keyboard::Key::C:
		p_os << "C"; break;
	case spk::Keyboard::Key::D:
		p_os << "D"; break;
	case spk::Keyboard::Key::E:
		p_os << "E"; break;
	case spk::Keyboard::Key::F:
		p_os << "F"; break;
	case spk::Keyboard::Key::G:
		p_os << "G"; break;
	case spk::Keyboard::Key::H:
		p_os << "H"; break;
	case spk::Keyboard::Key::I:
		p_os << "I"; break;
	case spk::Keyboard::Key::J:
		p_os << "J"; break;
	case spk::Keyboard::Key::K:
		p_os << "K"; break;
	case spk::Keyboard::Key::L:
		p_os << "L"; break;
	case spk::Keyboard::Key::M:
		p_os << "M"; break;
	case spk::Keyboard::Key::N:
		p_os << "N"; break;
	case spk::Keyboard::Key::O:
		p_os << "O"; break;
	case spk::Keyboard::Key::P:
		p_os << "P"; break;
	case spk::Keyboard::Key::Q:
		p_os << "Q"; break;
	case spk::Keyboard::Key::R:
		p_os << "R"; break;
	case spk::Keyboard::Key::S:
		p_os << "S"; break;
	case spk::Keyboard::Key::T:
		p_os << "T"; break;
	case spk::Keyboard::Key::U:
		p_os << "U"; break;
	case spk::Keyboard::Key::V:
		p_os << "V"; break;
	case spk::Keyboard::Key::W:
		p_os << "W"; break;
	case spk::Keyboard::Key::X:
		p_os << "X"; break;
	case spk::Keyboard::Key::Y:
		p_os << "Y"; break;
	case spk::Keyboard::Key::Z:
		p_os << "Z"; break;
	case spk::Keyboard::Key::LeftWindows:
		p_os << "LeftWindows"; break;
	case spk::Keyboard::Key::RightWindows:
		p_os << "RightWindows"; break;
	case spk::Keyboard::Key::App:
		p_os << "App"; break;
	case spk::Keyboard::Key::Sleep:
		p_os << "Sleep"; break;
	case spk::Keyboard::Key::Numpad0:
		p_os << "Numpad0"; break;
	case spk::Keyboard::Key::Numpad1:
		p_os << "Numpad1"; break;
	case spk::Keyboard::Key::Numpad2:
		p_os << "Numpad2"; break;
	case spk::Keyboard::Key::Numpad3:
		p_os << "Numpad3"; break;
	case spk::Keyboard::Key::Numpad4:
		p_os << "Numpad4"; break;
	case spk::Keyboard::Key::Numpad5:
		p_os << "Numpad5"; break;
	case spk::Keyboard::Key::Numpad6:
		p_os << "Numpad6"; break;
	case spk::Keyboard::Key::Numpad7:
		p_os << "Numpad7"; break;
	case spk::Keyboard::Key::Numpad8:
		p_os << "Numpad8"; break;
	case spk::Keyboard::Key::Numpad9:
		p_os << "Numpad9"; break;
	case spk::Keyboard::Key::NumpadMultiply:
		p_os << "NumpadMultiply"; break;
	case spk::Keyboard::Key::NumpadPlus:
		p_os << "NumpadPlus"; break;
	case spk::Keyboard::Key::NumpadSeparator:
		p_os << "NumpadSeparator"; break;
	case spk::Keyboard::Key::NumpadMinus:
		p_os << "NumpadMinus"; break;
	case spk::Keyboard::Key::NumpadDecimal:
		p_os << "NumpadDecimal"; break;
	case spk::Keyboard::Key::NumpadDivide:
		p_os << "NumpadDivide"; break;
	case spk::Keyboard::Key::F1:
		p_os << "F1"; break;
	case spk::Keyboard::Key::F2:
		p_os << "F2"; break;
	case spk::Keyboard::Key::F3:
		p_os << "F3"; break;
	case spk::Keyboard::Key::F4:
		p_os << "F4"; break;
	case spk::Keyboard::Key::F5:
		p_os << "F5"; break;
	case spk::Keyboard::Key::F6:
		p_os << "F6"; break;
	case spk::Keyboard::Key::F7:
		p_os << "F7"; break;
	case spk::Keyboard::Key::F8:
		p_os << "F8"; break;
	case spk::Keyboard::Key::F9:
		p_os << "F9"; break;
	case spk::Keyboard::Key::F10:
		p_os << "F10"; break;
	case spk::Keyboard::Key::F11:
		p_os << "F11"; break;
	case spk::Keyboard::Key::F12:
		p_os << "F12"; break;
	case spk::Keyboard::Key::F13:
		p_os << "F13"; break;
	case spk::Keyboard::Key::F14:
		p_os << "F14"; break;
	case spk::Keyboard::Key::F15:
		p_os << "F15"; break;
	case spk::Keyboard::Key::F16:
		p_os << "F16"; break;
	case spk::Keyboard::Key::F17:
		p_os << "F17"; break;
	case spk::Keyboard::Key::F18:
		p_os << "F18"; break;
	case spk::Keyboard::Key::F19:
		p_os << "F19"; break;
	case spk::Keyboard::Key::F20:
		p_os << "F20"; break;
	case spk::Keyboard::Key::F21:
		p_os << "F21"; break;
	case spk::Keyboard::Key::F22:
		p_os << "F22"; break;
	case spk::Keyboard::Key::F23:
		p_os << "F23"; break;
	case spk::Keyboard::Key::F24:
		p_os << "F24"; break;
	case spk::Keyboard::Key::NumLock:
		p_os << "NumLock"; break;
	case spk::Keyboard::Key::Scroll:
		p_os << "Scroll"; break;
	case spk::Keyboard::Key::LeftShift:
		p_os << "LeftShift"; break;
	case spk::Keyboard::Key::RightShift:
		p_os << "RightShift"; break;
	case spk::Keyboard::Key::LeftControl:
		p_os << "LeftControl"; break;
	case spk::Keyboard::Key::RightControl:
		p_os << "RightControl"; break;
	case spk::Keyboard::Key::LeftMenu:
		p_os << "LeftMenu"; break;
	case spk::Keyboard::Key::RightMenu:
		p_os << "RightMenu"; break;
	case spk::Keyboard::Key::SemiColon:
		p_os << "SemiColon"; break;
	case spk::Keyboard::Key::Plus:
		p_os << "Plus"; break;
	case spk::Keyboard::Key::Comma:
		p_os << "Comma"; break;
	case spk::Keyboard::Key::Minus:
		p_os << "Minus"; break;
	case spk::Keyboard::Key::Period:
		p_os << "Period"; break;
	case spk::Keyboard::Key::QuestionMark:
		p_os << "QuestionMark"; break;
	case spk::Keyboard::Key::Tilde:
		p_os << "Tilde"; break;
	case spk::Keyboard::Key::LeftBracket:
		p_os << "LeftBracket"; break;
	case spk::Keyboard::Key::VerticalLine:
		p_os << "VerticalLine"; break;
	case spk::Keyboard::Key::RightBracket:
		p_os << "RightBracket"; break;
	case spk::Keyboard::Key::Quote:
		p_os << "Quote"; break;
	case spk::Keyboard::Key::Unknow:
		p_os << "Unknow"; break;
	case spk::Keyboard::Key::AngleBracket:
		p_os << "AngleBracket"; break;
	case spk::Keyboard::Key::Process:
		p_os << "Process"; break;
	}
	return (p_os);
}

std::wostream& operator << (std::wostream& p_os, const spk::Keyboard::Key& p_key)
{

	switch (p_key)
	{
	case spk::Keyboard::Key::Backspace:
		p_os << "Backspace"; break;
	case spk::Keyboard::Key::Tab:
		p_os << "Tab"; break;
	case spk::Keyboard::Key::Clear:
		p_os << "Clear"; break;
	case spk::Keyboard::Key::Return:
		p_os << "Return"; break;
	case spk::Keyboard::Key::Shift:
		p_os << "Shift"; break;
	case spk::Keyboard::Key::Control:
		p_os << "Control"; break;
	case spk::Keyboard::Key::Alt:
		p_os << "Alt"; break;
	case spk::Keyboard::Key::Pause:
		p_os << "Pause"; break;
	case spk::Keyboard::Key::Capslock:
		p_os << "Capslock"; break;
	case spk::Keyboard::Key::Escape:
		p_os << "Escape"; break;
	case spk::Keyboard::Key::Convert:
		p_os << "Convert"; break;
	case spk::Keyboard::Key::Non_convert:
		p_os << "Non_convert"; break;
	case spk::Keyboard::Key::Accept:
		p_os << "Accept"; break;
	case spk::Keyboard::Key::Mode_change:
		p_os << "Mode_change"; break;
	case spk::Keyboard::Key::Space:
		p_os << "Space"; break;
	case spk::Keyboard::Key::Prior:
		p_os << "Prior"; break;
	case spk::Keyboard::Key::Next:
		p_os << "Next"; break;
	case spk::Keyboard::Key::End:
		p_os << "End"; break;
	case spk::Keyboard::Key::Home:
		p_os << "Home"; break;
	case spk::Keyboard::Key::LeftArrow:
		p_os << "LeftArrow"; break;
	case spk::Keyboard::Key::UpArrow:
		p_os << "UpArrow"; break;
	case spk::Keyboard::Key::RightArrow:
		p_os << "RightArrow"; break;
	case spk::Keyboard::Key::DownArrow:
		p_os << "DownArrow"; break;
	case spk::Keyboard::Key::Select:
		p_os << "Select"; break;
	case spk::Keyboard::Key::Print:
		p_os << "Print"; break;
	case spk::Keyboard::Key::Execute:
		p_os << "Execute"; break;
	case spk::Keyboard::Key::Snapshot:
		p_os << "Snapshot"; break;
	case spk::Keyboard::Key::Insert:
		p_os << "Insert"; break;
	case spk::Keyboard::Key::Delete:
		p_os << "Delete"; break;
	case spk::Keyboard::Key::Help:
		p_os << "Help"; break;
	case spk::Keyboard::Key::Key0:
		p_os << "Key0"; break;
	case spk::Keyboard::Key::Key1:
		p_os << "Key1"; break;
	case spk::Keyboard::Key::Key2:
		p_os << "Key2"; break;
	case spk::Keyboard::Key::Key3:
		p_os << "Key3"; break;
	case spk::Keyboard::Key::Key4:
		p_os << "Key4"; break;
	case spk::Keyboard::Key::Key5:
		p_os << "Key5"; break;
	case spk::Keyboard::Key::Key6:
		p_os << "Key6"; break;
	case spk::Keyboard::Key::Key7:
		p_os << "Key7"; break;
	case spk::Keyboard::Key::Key8:
		p_os << "Key8"; break;
	case spk::Keyboard::Key::Key9:
		p_os << "Key9"; break;
	case spk::Keyboard::Key::A:
		p_os << "A"; break;
	case spk::Keyboard::Key::B:
		p_os << "B"; break;
	case spk::Keyboard::Key::C:
		p_os << "C"; break;
	case spk::Keyboard::Key::D:
		p_os << "D"; break;
	case spk::Keyboard::Key::E:
		p_os << "E"; break;
	case spk::Keyboard::Key::F:
		p_os << "F"; break;
	case spk::Keyboard::Key::G:
		p_os << "G"; break;
	case spk::Keyboard::Key::H:
		p_os << "H"; break;
	case spk::Keyboard::Key::I:
		p_os << "I"; break;
	case spk::Keyboard::Key::J:
		p_os << "J"; break;
	case spk::Keyboard::Key::K:
		p_os << "K"; break;
	case spk::Keyboard::Key::L:
		p_os << "L"; break;
	case spk::Keyboard::Key::M:
		p_os << "M"; break;
	case spk::Keyboard::Key::N:
		p_os << "N"; break;
	case spk::Keyboard::Key::O:
		p_os << "O"; break;
	case spk::Keyboard::Key::P:
		p_os << "P"; break;
	case spk::Keyboard::Key::Q:
		p_os << "Q"; break;
	case spk::Keyboard::Key::R:
		p_os << "R"; break;
	case spk::Keyboard::Key::S:
		p_os << "S"; break;
	case spk::Keyboard::Key::T:
		p_os << "T"; break;
	case spk::Keyboard::Key::U:
		p_os << "U"; break;
	case spk::Keyboard::Key::V:
		p_os << "V"; break;
	case spk::Keyboard::Key::W:
		p_os << "W"; break;
	case spk::Keyboard::Key::X:
		p_os << "X"; break;
	case spk::Keyboard::Key::Y:
		p_os << "Y"; break;
	case spk::Keyboard::Key::Z:
		p_os << "Z"; break;
	case spk::Keyboard::Key::LeftWindows:
		p_os << "LeftWindows"; break;
	case spk::Keyboard::Key::RightWindows:
		p_os << "RightWindows"; break;
	case spk::Keyboard::Key::App:
		p_os << "App"; break;
	case spk::Keyboard::Key::Sleep:
		p_os << "Sleep"; break;
	case spk::Keyboard::Key::Numpad0:
		p_os << "Numpad0"; break;
	case spk::Keyboard::Key::Numpad1:
		p_os << "Numpad1"; break;
	case spk::Keyboard::Key::Numpad2:
		p_os << "Numpad2"; break;
	case spk::Keyboard::Key::Numpad3:
		p_os << "Numpad3"; break;
	case spk::Keyboard::Key::Numpad4:
		p_os << "Numpad4"; break;
	case spk::Keyboard::Key::Numpad5:
		p_os << "Numpad5"; break;
	case spk::Keyboard::Key::Numpad6:
		p_os << "Numpad6"; break;
	case spk::Keyboard::Key::Numpad7:
		p_os << "Numpad7"; break;
	case spk::Keyboard::Key::Numpad8:
		p_os << "Numpad8"; break;
	case spk::Keyboard::Key::Numpad9:
		p_os << "Numpad9"; break;
	case spk::Keyboard::Key::NumpadMultiply:
		p_os << "NumpadMultiply"; break;
	case spk::Keyboard::Key::NumpadPlus:
		p_os << "NumpadPlus"; break;
	case spk::Keyboard::Key::NumpadSeparator:
		p_os << "NumpadSeparator"; break;
	case spk::Keyboard::Key::NumpadMinus:
		p_os << "NumpadMinus"; break;
	case spk::Keyboard::Key::NumpadDecimal:
		p_os << "NumpadDecimal"; break;
	case spk::Keyboard::Key::NumpadDivide:
		p_os << "NumpadDivide"; break;
	case spk::Keyboard::Key::F1:
		p_os << "F1"; break;
	case spk::Keyboard::Key::F2:
		p_os << "F2"; break;
	case spk::Keyboard::Key::F3:
		p_os << "F3"; break;
	case spk::Keyboard::Key::F4:
		p_os << "F4"; break;
	case spk::Keyboard::Key::F5:
		p_os << "F5"; break;
	case spk::Keyboard::Key::F6:
		p_os << "F6"; break;
	case spk::Keyboard::Key::F7:
		p_os << "F7"; break;
	case spk::Keyboard::Key::F8:
		p_os << "F8"; break;
	case spk::Keyboard::Key::F9:
		p_os << "F9"; break;
	case spk::Keyboard::Key::F10:
		p_os << "F10"; break;
	case spk::Keyboard::Key::F11:
		p_os << "F11"; break;
	case spk::Keyboard::Key::F12:
		p_os << "F12"; break;
	case spk::Keyboard::Key::F13:
		p_os << "F13"; break;
	case spk::Keyboard::Key::F14:
		p_os << "F14"; break;
	case spk::Keyboard::Key::F15:
		p_os << "F15"; break;
	case spk::Keyboard::Key::F16:
		p_os << "F16"; break;
	case spk::Keyboard::Key::F17:
		p_os << "F17"; break;
	case spk::Keyboard::Key::F18:
		p_os << "F18"; break;
	case spk::Keyboard::Key::F19:
		p_os << "F19"; break;
	case spk::Keyboard::Key::F20:
		p_os << "F20"; break;
	case spk::Keyboard::Key::F21:
		p_os << "F21"; break;
	case spk::Keyboard::Key::F22:
		p_os << "F22"; break;
	case spk::Keyboard::Key::F23:
		p_os << "F23"; break;
	case spk::Keyboard::Key::F24:
		p_os << "F24"; break;
	case spk::Keyboard::Key::NumLock:
		p_os << "NumLock"; break;
	case spk::Keyboard::Key::Scroll:
		p_os << "Scroll"; break;
	case spk::Keyboard::Key::LeftShift:
		p_os << "LeftShift"; break;
	case spk::Keyboard::Key::RightShift:
		p_os << "RightShift"; break;
	case spk::Keyboard::Key::LeftControl:
		p_os << "LeftControl"; break;
	case spk::Keyboard::Key::RightControl:
		p_os << "RightControl"; break;
	case spk::Keyboard::Key::LeftMenu:
		p_os << "LeftMenu"; break;
	case spk::Keyboard::Key::RightMenu:
		p_os << "RightMenu"; break;
	case spk::Keyboard::Key::SemiColon:
		p_os << "SemiColon"; break;
	case spk::Keyboard::Key::Plus:
		p_os << "Plus"; break;
	case spk::Keyboard::Key::Comma:
		p_os << "Comma"; break;
	case spk::Keyboard::Key::Minus:
		p_os << "Minus"; break;
	case spk::Keyboard::Key::Period:
		p_os << "Period"; break;
	case spk::Keyboard::Key::QuestionMark:
		p_os << "QuestionMark"; break;
	case spk::Keyboard::Key::Tilde:
		p_os << "Tilde"; break;
	case spk::Keyboard::Key::LeftBracket:
		p_os << "LeftBracket"; break;
	case spk::Keyboard::Key::VerticalLine:
		p_os << "VerticalLine"; break;
	case spk::Keyboard::Key::RightBracket:
		p_os << "RightBracket"; break;
	case spk::Keyboard::Key::Quote:
		p_os << "Quote"; break;
	case spk::Keyboard::Key::Unknow:
		p_os << "Unknow"; break;
	case spk::Keyboard::Key::AngleBracket:
		p_os << "AngleBracket"; break;
	case spk::Keyboard::Key::Process:
		p_os << "Process"; break;
	}
	return (p_os);
}