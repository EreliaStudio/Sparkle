#include "ui/widget/screen.hpp"

#include <utility>

namespace spk
{
	Screen *Screen::_activeScreen = nullptr;

	Screen::Screen(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		_activationContract = subscribeToActivation([this]() {
			if (_activeScreen != nullptr && _activeScreen != this)
			{
				_activeScreen->deactivate();
			}
			_activeScreen = this;
		});
	}

	Screen::~Screen()
	{
		if (_activeScreen == this)
		{
			_activeScreen = nullptr;
		}
	}

	Screen *Screen::activeScreen() noexcept
	{
		return _activeScreen;
	}
}
