#include "input/spk_keyboard.hpp"

#include "iostream"

namespace spk
{
	Keyboard::Keyboard()
	{
		for (size_t i = 0; i < 255; i++)
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
}