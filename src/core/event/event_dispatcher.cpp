#include "core/event/event_dispatcher.hpp"

namespace spk
{
	void EventDispatcher::dispatch(WindowResizedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onWindowResizedEvent(event);
		}
	}

	void EventDispatcher::dispatch(WindowMovedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onWindowMovedEvent(event);
		}
	}

	void EventDispatcher::dispatch(WindowFocusGainedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onWindowFocusGainedEvent(event);
		}
	}

	void EventDispatcher::dispatch(WindowFocusLostEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onWindowFocusLostEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseEnteredEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseEnteredEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseLeftEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseLeftEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseMovedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseMovedEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseWheelScrolledEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseWheelScrolledEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseButtonPressedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseButtonPressedEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseButtonReleasedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseButtonReleasedEvent(event);
		}
	}

	void EventDispatcher::dispatch(MouseButtonDoubleClickedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onMouseButtonDoubleClickedEvent(event);
		}
	}

	void EventDispatcher::dispatch(KeyPressedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onKeyPressedEvent(event);
		}
	}

	void EventDispatcher::dispatch(KeyReleasedEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onKeyReleasedEvent(event);
		}
	}

	void EventDispatcher::dispatch(TextInputEvent &event)
	{
		if (!_isAcceptingInteraction() || event.consumed)
		{
			return;
		}

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr && !event.consumed)
			{
				dispatcher->dispatch(event);
			}
		});

		if (!event.consumed)
		{
			_onTextInputEvent(event);
		}
	}

	void EventDispatcher::observePointer(MouseMovedEvent &event)
	{
		if (!_isAcceptingInteraction())
		{
			return;
		}

		_onPassiveMouseMovedEvent(event);

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr)
			{
				dispatcher->observePointer(event);
			}
		});
	}

	void EventDispatcher::observePointer(MouseButtonPressedEvent &event)
	{
		if (!_isAcceptingInteraction())
		{
			return;
		}

		_onPassiveMouseButtonPressedEvent(event);

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr)
			{
				dispatcher->observePointer(event);
			}
		});
	}

	void EventDispatcher::observeKeyboard(KeyPressedEvent &event)
	{
		if (!_isAcceptingInteraction())
		{
			return;
		}

		_onPassiveKeyPressedEvent(event);

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr)
			{
				dispatcher->observeKeyboard(event);
			}
		});
	}

	void EventDispatcher::observeKeyboard(KeyReleasedEvent &event)
	{
		if (!_isAcceptingInteraction())
		{
			return;
		}

		_onPassiveKeyReleasedEvent(event);

		_propagateInteraction([&event](EventDispatcher *dispatcher) {
			if (dispatcher != nullptr)
			{
				dispatcher->observeKeyboard(event);
			}
		});
	}

	void EventDispatcher::_onWindowResizedEvent(WindowResizedEvent &)
	{
	}

	void EventDispatcher::_onWindowMovedEvent(WindowMovedEvent &)
	{
	}

	void EventDispatcher::_onWindowFocusGainedEvent(WindowFocusGainedEvent &)
	{
	}

	void EventDispatcher::_onWindowFocusLostEvent(WindowFocusLostEvent &)
	{
	}

	void EventDispatcher::_onMouseEnteredEvent(MouseEnteredEvent &)
	{
	}

	void EventDispatcher::_onMouseLeftEvent(MouseLeftEvent &)
	{
	}

	void EventDispatcher::_onMouseMovedEvent(MouseMovedEvent &)
	{
	}

	void EventDispatcher::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &)
	{
	}

	void EventDispatcher::_onMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
	}

	void EventDispatcher::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &)
	{
	}

	void EventDispatcher::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &)
	{
	}

	void EventDispatcher::_onKeyPressedEvent(KeyPressedEvent &)
	{
	}

	void EventDispatcher::_onKeyReleasedEvent(KeyReleasedEvent &)
	{
	}

	void EventDispatcher::_onTextInputEvent(TextInputEvent &)
	{
	}

	void EventDispatcher::_onPassiveMouseMovedEvent(MouseMovedEvent &)
	{
	}

	void EventDispatcher::_onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
	}

	void EventDispatcher::_onPassiveKeyPressedEvent(KeyPressedEvent &)
	{
	}

	void EventDispatcher::_onPassiveKeyReleasedEvent(KeyReleasedEvent &)
	{
	}
}