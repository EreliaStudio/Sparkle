#include "input/spk_keyboard.hpp"

#include "spk_basic_functions.hpp"

namespace spk
{
    Keyboard::Keyboard()
    {
        for (size_t i = 0; i < Keyboard::Key::Maximum; i++)
        {
            _keys[i] = kUp;
        }
        _char = L'\0';
    }

    Keyboard::~Keyboard()
    {
    }

    void Keyboard::setChar(const wchar_t& p_char)
    {
        _char = p_char;
    }

    void Keyboard::pressKey(uint32_t p_key)
    {
        if (_keys[p_key] != kDown)
        {
			std::cout << "Pressing key id : " << std::hex << p_key << std::dec << std::endl;
            _keys[p_key] = kPressed;
            _keysToUpdate.push_back(p_key);
        }
    }

    void Keyboard::releaseKey(uint32_t p_key)
    {
        if (_keys[p_key] != kUp)
        {
            _keys[p_key] = kReleased;
            _keysToUpdate.push_back(p_key);
        }
    }

    void Keyboard::update()
    {
        for (const auto& key : _keysToUpdate)
        {
            if (_keys[key] == kPressed)
                _keys[key] = kDown;
            else if (_keys[key] == kReleased)
                _keys[key] = kUp;
        }
        _char = L'\0';
        _keysToUpdate.clear();
    }

    wchar_t Keyboard::getChar() const
    {
        return (_char);
    }

    InputState Keyboard::getKey(Key p_key) const
    {
        return (static_cast<InputState>(_keys[static_cast<size_t>(p_key)].load()));
    }

    std::ostream& operator << (std::ostream& p_os, const Keyboard::Key& p_key)
    {
        switch (p_key)
        {
            case Keyboard::Key::Backspace:
                p_os << "Backspace"; break;
            case Keyboard::Key::Tab:
                p_os << "Tab"; break;
            case Keyboard::Key::Clear:
                p_os << "Clear"; break;
            case Keyboard::Key::Return:
                p_os << "Return"; break;
            case Keyboard::Key::Shift:
                p_os << "Shift"; break;
            case Keyboard::Key::Control:
                p_os << "Control"; break;
            case Keyboard::Key::Alt:
                p_os << "Alt"; break;
            case Keyboard::Key::Pause:
                p_os << "Pause"; break;
            case Keyboard::Key::Capslock:
                p_os << "Capslock"; break;
            case Keyboard::Key::Escape:
                p_os << "Escape"; break;
            case Keyboard::Key::Convert:
                p_os << "Convert"; break;
            case Keyboard::Key::Non_convert:
                p_os << "Non_convert"; break;
            case Keyboard::Key::Accept:
                p_os << "Accept"; break;
            case Keyboard::Key::Mode_change:
                p_os << "Mode_change"; break;
            case Keyboard::Key::Space:
                p_os << "Space"; break;
            case Keyboard::Key::Prior:
                p_os << "Prior"; break;
            case Keyboard::Key::Next:
                p_os << "Next"; break;
            case Keyboard::Key::End:
                p_os << "End"; break;
            case Keyboard::Key::Home:
                p_os << "Home"; break;
            case Keyboard::Key::LeftArrow:
                p_os << "LeftArrow"; break;
            case Keyboard::Key::UpArrow:
                p_os << "UpArrow"; break;
            case Keyboard::Key::RightArrow:
                p_os << "RightArrow"; break;
            case Keyboard::Key::DownArrow:
                p_os << "DownArrow"; break;
            case Keyboard::Key::Select:
                p_os << "Select"; break;
            case Keyboard::Key::Print:
                p_os << "Print"; break;
            case Keyboard::Key::Execute:
                p_os << "Execute"; break;
            case Keyboard::Key::Snapshot:
                p_os << "Snapshot"; break;
            case Keyboard::Key::Insert:
                p_os << "Insert"; break;
            case Keyboard::Key::Delete:
                p_os << "Delete"; break;
            case Keyboard::Key::Help:
                p_os << "Help"; break;
            case Keyboard::Key::Key0:
                p_os << "Key0"; break;
            case Keyboard::Key::Key1:
                p_os << "Key1"; break;
            case Keyboard::Key::Key2:
                p_os << "Key2"; break;
            case Keyboard::Key::Key3:
                p_os << "Key3"; break;
            case Keyboard::Key::Key4:
                p_os << "Key4"; break;
            case Keyboard::Key::Key5:
                p_os << "Key5"; break;
            case Keyboard::Key::Key6:
                p_os << "Key6"; break;
            case Keyboard::Key::Key7:
                p_os << "Key7"; break;
            case Keyboard::Key::Key8:
                p_os << "Key8"; break;
            case Keyboard::Key::Key9:
                p_os << "Key9"; break;
            case Keyboard::Key::A:
                p_os << "A"; break;
            case Keyboard::Key::B:
                p_os << "B"; break;
            case Keyboard::Key::C:
                p_os << "C"; break;
            case Keyboard::Key::D:
                p_os << "D"; break;
            case Keyboard::Key::E:
                p_os << "E"; break;
            case Keyboard::Key::F:
                p_os << "F"; break;
            case Keyboard::Key::G:
                p_os << "G"; break;
            case Keyboard::Key::H:
                p_os << "H"; break;
            case Keyboard::Key::I:
                p_os << "I"; break;
            case Keyboard::Key::J:
                p_os << "J"; break;
            case Keyboard::Key::K:
                p_os << "K"; break;
            case Keyboard::Key::L:
                p_os << "L"; break;
            case Keyboard::Key::M:
                p_os << "M"; break;
            case Keyboard::Key::N:
                p_os << "N"; break;
            case Keyboard::Key::O:
                p_os << "O"; break;
            case Keyboard::Key::P:
                p_os << "P"; break;
            case Keyboard::Key::Q:
                p_os << "Q"; break;
            case Keyboard::Key::R:
                p_os << "R"; break;
            case Keyboard::Key::S:
                p_os << "S"; break;
            case Keyboard::Key::T:
                p_os << "T"; break;
            case Keyboard::Key::U:
                p_os << "U"; break;
            case Keyboard::Key::V:
                p_os << "V"; break;
            case Keyboard::Key::W:
                p_os << "W"; break;
            case Keyboard::Key::X:
                p_os << "X"; break;
            case Keyboard::Key::Y:
                p_os << "Y"; break;
            case Keyboard::Key::Z:
                p_os << "Z"; break;
            case Keyboard::Key::LeftWindows:
                p_os << "LeftWindows"; break;
            case Keyboard::Key::RightWindows:
                p_os << "RightWindows"; break;
            case Keyboard::Key::App:
                p_os << "App"; break;
            case Keyboard::Key::Sleep:
                p_os << "Sleep"; break;
            case Keyboard::Key::Numpad0:
                p_os << "Numpad0"; break;
            case Keyboard::Key::Numpad1:
                p_os << "Numpad1"; break;
            case Keyboard::Key::Numpad2:
                p_os << "Numpad2"; break;
            case Keyboard::Key::Numpad3:
                p_os << "Numpad3"; break;
            case Keyboard::Key::Numpad4:
                p_os << "Numpad4"; break;
            case Keyboard::Key::Numpad5:
                p_os << "Numpad5"; break;
            case Keyboard::Key::Numpad6:
                p_os << "Numpad6"; break;
            case Keyboard::Key::Numpad7:
                p_os << "Numpad7"; break;
            case Keyboard::Key::Numpad8:
                p_os << "Numpad8"; break;
            case Keyboard::Key::Numpad9:
                p_os << "Numpad9"; break;
            case Keyboard::Key::NumpadMultiply:
                p_os << "NumpadMultiply"; break;
            case Keyboard::Key::NumpadPlus:
                p_os << "NumpadPlus"; break;
            case Keyboard::Key::NumpadSeparator:
                p_os << "NumpadSeparator"; break;
            case Keyboard::Key::NumpadMinus:
                p_os << "NumpadMinus"; break;
            case Keyboard::Key::NumpadDecimal:
                p_os << "NumpadDecimal"; break;
            case Keyboard::Key::NumpadDivide:
                p_os << "NumpadDivide"; break;
            case Keyboard::Key::F1:
                p_os << "F1"; break;
            case Keyboard::Key::F2:
                p_os << "F2"; break;
            case Keyboard::Key::F3:
                p_os << "F3"; break;
            case Keyboard::Key::F4:
                p_os << "F4"; break;
            case Keyboard::Key::F5:
                p_os << "F5"; break;
            case Keyboard::Key::F6:
                p_os << "F6"; break;
            case Keyboard::Key::F7:
                p_os << "F7"; break;
            case Keyboard::Key::F8:
                p_os << "F8"; break;
            case Keyboard::Key::F9:
                p_os << "F9"; break;
            case Keyboard::Key::F10:
                p_os << "F10"; break;
            case Keyboard::Key::F11:
                p_os << "F11"; break;
            case Keyboard::Key::F12:
                p_os << "F12"; break;
            case Keyboard::Key::F13:
                p_os << "F13"; break;
            case Keyboard::Key::F14:
                p_os << "F14"; break;
            case Keyboard::Key::F15:
                p_os << "F15"; break;
            case Keyboard::Key::F16:
                p_os << "F16"; break;
            case Keyboard::Key::F17:
                p_os << "F17"; break;
            case Keyboard::Key::F18:
                p_os << "F18"; break;
            case Keyboard::Key::F19:
                p_os << "F19"; break;
            case Keyboard::Key::F20:
                p_os << "F20"; break;
            case Keyboard::Key::F21:
                p_os << "F21"; break;
            case Keyboard::Key::F22:
                p_os << "F22"; break;
            case Keyboard::Key::F23:
                p_os << "F23"; break;
            case Keyboard::Key::F24:
                p_os << "F24"; break;
            case Keyboard::Key::NumLock:
                p_os << "NumLock"; break;
            case Keyboard::Key::Scroll:
                p_os << "Scroll"; break;
            case Keyboard::Key::LeftShift:
                p_os << "LeftShift"; break;
            case Keyboard::Key::RightShift:
                p_os << "RightShift"; break;
            case Keyboard::Key::LeftControl:
                p_os << "LeftControl"; break;
            case Keyboard::Key::RightControl:
                p_os << "RightControl"; break;
            case Keyboard::Key::LeftMenu:
                p_os << "LeftMenu"; break;
            case Keyboard::Key::RightMenu:
                p_os << "RightMenu"; break;
            case Keyboard::Key::SemiColon:
                p_os << "SemiColon"; break;
            case Keyboard::Key::Plus:
                p_os << "Plus"; break;
            case Keyboard::Key::Comma:
                p_os << "Comma"; break;
            case Keyboard::Key::Minus:
                p_os << "Minus"; break;
            case Keyboard::Key::Period:
                p_os << "Period"; break;
            case Keyboard::Key::QuestionMark:
                p_os << "QuestionMark"; break;
            case Keyboard::Key::Tilde:
                p_os << "Tilde"; break;
            case Keyboard::Key::LeftBracket:
                p_os << "LeftBracket"; break;
            case Keyboard::Key::VerticalLine:
                p_os << "VerticalLine"; break;
            case Keyboard::Key::RightBracket:
                p_os << "RightBracket"; break;
            case Keyboard::Key::Quote:
                p_os << "Quote"; break;
            case Keyboard::Key::Unknow:
                p_os << "Unknow"; break;
            case Keyboard::Key::AngleBracket:
                p_os << "AngleBracket"; break;
            case Keyboard::Key::Process:
                p_os << "Process"; break;
            case Keyboard::Key::Maximum:
                p_os << "Maximum"; break;
            default:
                p_os << "Unknow key"; break;
        }
        return (p_os);
    }
}