#include "internal/application_internal.hpp"

#include <cstdint>
#include <utility>
#include <variant>

#include "input_state.hpp"
#include "exception.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"
#include "widget.hpp"

#include <iostream>

namespace spk
{
	Application::UpdateRuntime::UpdateRuntime(
		spk::ThreadSafeFIFO<EventRecord>::Consumer eventRecordConsumer,
		spk::ThreadSafeFIFO<UpdateRequest>::Consumer updateRequestConsumer) :
		Runtime("update"),
		_eventRecordConsumer(std::move(eventRecordConsumer)),
		_updateRequestConsumer(std::move(updateRequestConsumer)),
		_startTime(std::chrono::steady_clock::now()),
		_currentTime(_startTime)
	{
	}

	void Application::UpdateRuntime::_registerSnapshotProducer(
		const Window::Identifier &identifier,
		spk::ThreadSafeSlot<spk::RenderSnapshot>::Producer producer,
		std::shared_ptr<std::atomic_bool> isRequested)
	{
		auto [it, inserted] = _renderSnapshotEntries.emplace(identifier, RenderSnapshotEntry{std::move(producer), std::move(isRequested)});
		if (!inserted)
		{
			throw std::logic_error("A render snapshot producer already exists for [" + identifier + "]");
		}
	}

	template <typename TEvent>
	void Application::UpdateRuntime::_applyFocusChanges(const TEvent &event, Window::State &state)
	{
		for (std::size_t index = 0; index < FocusMode::ChannelCount; ++index)
		{
			const auto channel = static_cast<FocusMode::Channel>(index);
			const auto &change = event.focusChange(channel);
			if (!change.has_value())
			{
				continue;
			}
			if (change->type == FocusMode::ChangeType::Take)
			{
				state.takeFocus(channel, change->widget);
			}
			else
			{
				state.releaseFocus(channel, change->widget);
			}
		}
	}

	template <typename TRecord>
	void Application::UpdateRuntime::_dispatch(const TRecord &record, Window::State &state)
	{
		Event<TRecord> event(record);
		state.root().dispatch(event);
		_applyFocusChanges(event, state);
	}

	template <typename TRecord>
	void Application::UpdateRuntime::_dispatchMouse(const TRecord &record, Window::State &state)
	{
		DeviceEvent<TRecord, spk::Mouse> event(record, state.mouse());
		state.dispatchRoot(FocusMode::Channel::Mouse).dispatch(event);
		_applyFocusChanges(event, state);
	}

	template <typename TRecord>
	void Application::UpdateRuntime::_dispatchKeyboard(const TRecord &record, Window::State &state)
	{
		DeviceEvent<TRecord, spk::Keyboard> event(record, state.keyboard());
		state.dispatchRoot(FocusMode::Channel::Keyboard).dispatch(event);
		_applyFocusChanges(event, state);
	}

	template <typename TRecord>
	Window::State *Application::UpdateRuntime::_state(const TRecord &record)
	{
		return tryGet(record.windowIdentifier);
	}

	void Application::UpdateRuntime::_consume(const WindowResizedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->root().resize(spk::Rect2D{.anchor = spk::Vector2Int(0, 0), .size = spk::Vector2UInt(static_cast<std::uint32_t>(record.size.x), static_cast<std::uint32_t>(record.size.y))});
		_dispatch(record, *state);
	}

	void Application::UpdateRuntime::_consume(const WindowMovedRecord &record)
	{
		if (Window::State *state = _state(record))
		{
			_dispatch(record, *state);
		}
	}

	void Application::UpdateRuntime::_consume(const WindowFocusGainedRecord &record)
	{
		if (Window::State *state = _state(record))
		{
			_dispatch(record, *state);
		}
	}

	void Application::UpdateRuntime::_resetInput(Window::State &state)
	{
		state.keyboard().keys.fill(spk::InputState::Up);
		state.mouse().buttons.fill(spk::InputState::Up);
	}

	void Application::UpdateRuntime::_consume(const WindowFocusLostRecord &record)
	{
		if (Window::State *state = _state(record))
		{
			_resetInput(*state);
			_dispatch(record, *state);
		}
	}

	void Application::UpdateRuntime::_consume(const MouseEnteredRecord &record)
	{
		if (Window::State *state = _state(record))
		{
			_dispatch(record, *state);
		}
	}

	void Application::UpdateRuntime::_consume(const MouseLeftRecord &record)
	{
		if (Window::State *state = _state(record))
		{
			_dispatch(record, *state);
		}
	}

	void Application::UpdateRuntime::_consume(const MouseMovedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		spk::Mouse &mouse = state->mouse();
		const spk::Vector2Int delta = record.position - mouse.position;
		if (delta == spk::Vector2Int(0, 0))
		{
			mouse.deltaPosition = {};
			return;
		}
		mouse.deltaPosition = delta;
		mouse.position = record.position;
		_dispatchMouse(record, *state);
	}

	void Application::UpdateRuntime::_consume(const MouseWheelScrolledRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->mouse().wheel += record.value.y;
		_dispatchMouse(record, *state);
	}

	void Application::UpdateRuntime::_consume(const MouseButtonPressedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->mouse()[record.button] = spk::InputState::Down;
		_dispatchMouse(record, *state);
	}

	void Application::UpdateRuntime::_consume(const MouseButtonReleasedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->mouse()[record.button] = spk::InputState::Up;
		_dispatchMouse(record, *state);
	}

	void Application::UpdateRuntime::_consume(const MouseButtonDoubleClickedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->mouse()[record.button] = spk::InputState::Down;
		_dispatchMouse(record, *state);
	}

	void Application::UpdateRuntime::_consume(const KeyPressedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->keyboard()[record.key] = spk::InputState::Down;
		_dispatchKeyboard(record, *state);
	}

	void Application::UpdateRuntime::_consume(const KeyReleasedRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->keyboard()[record.key] = spk::InputState::Up;
		_dispatchKeyboard(record, *state);
	}

	void Application::UpdateRuntime::_consume(const TextInputRecord &record)
	{
		Window::State *state = _state(record);
		if (state == nullptr)
		{
			return;
		}
		state->keyboard().glyph = static_cast<char32_t>(record.glyph);
		_dispatchKeyboard(record, *state);
	}

	void Application::UpdateRuntime::_consume(const EventRecord &event)
	{
		std::visit([this](const auto &value) {
			_consume(value);
		},
				   event);
	}

	void Application::UpdateRuntime::_consume(const StateRegistrationRequest &request)
	{
		append(request.windowIdentifier, request.state);
		request.state->setBackgroundColor(request.backgroundColor);
		_registerSnapshotProducer(request.windowIdentifier, request.renderSnapshotProducer, request.isRequested);
		request.state->markReady();
	}

	void Application::UpdateRuntime::_consume(const StateDeletionRequest &request)
	{
		if (!contains(request.windowIdentifier))
		{
			return;
		}
		auto &state = object(request.windowIdentifier);
		release(state);
		_renderSnapshotEntries.erase(request.windowIdentifier);
		remove(request.windowIdentifier);
	}

	void Application::UpdateRuntime::_consumeEvents()
	{
		for (auto &event : _eventRecordConsumer.drain())
		{
			_consume(event);
		}
	}

	void Application::UpdateRuntime::_consumeRequests()
	{
		for (auto &request : _updateRequestConsumer.drain())
		{
			std::visit([this](const auto &value) {
				_consume(value);
			},
					   request);
		}
	}

	void Application::UpdateRuntime::_updateState(Window::State &state, UpdateContext &context)
	{
		state.root().updateState(context);
	}

	spk::RenderSnapshot Application::UpdateRuntime::_buildRenderSnapshot(const Window::Identifier &identifier, Window::State &state)
	{
		try
		{
			spk::RenderSnapshot::Builder builder;
			state.root().buildRenderSnapshot(builder);
			return builder.build();
		}
		catch (spk::Exception &exception)
		{
			exception.addContext("Exception while building render snapshot for window [" + identifier + "]");
			throw;
		}
		catch (...)
		{
			throw spk::Exception(
				"Exception while building render snapshot for window [" + identifier + "]",
				std::current_exception());
		}
	}

	bool Application::UpdateRuntime::_consumeSnapshotRequest(const Window::Identifier &identifier)
	{
		return _renderSnapshotEntries.at(identifier).isRequested->exchange(false, std::memory_order_acq_rel);
	}

	void Application::UpdateRuntime::_publishSnapshot(const Window::Identifier &identifier, spk::RenderSnapshot &&snapshot)
	{
		_renderSnapshotEntries.at(identifier).producer.publish(std::move(snapshot));
	}

	void Application::UpdateRuntime::consumeIncoming()
	{
		_consumeRequests();
		_consumeEvents();
	}

	void Application::UpdateRuntime::prepareCycle()
	{
		_currentTime = std::chrono::steady_clock::now();
		_deltaTime = _lastTime.has_value() ? _currentTime - *_lastTime : std::chrono::steady_clock::duration::zero();
	}

	void Application::UpdateRuntime::tickOnce(const Window::Identifier &identifier, Window::State &state)
	{
		UpdateContext context{
			.time = _currentTime - _startTime,
			.deltaTime = _deltaTime,
			.keyboard = state.keyboard(),
			.mouse = state.mouse()};
		try
		{
			_updateState(state, context);
		}
		catch (spk::Exception &exception)
		{
			exception.addContext("Exception while updating window [" + identifier + "]");
			throw;
		}
		catch (...)
		{
			throw spk::Exception(
				"Exception while updating window [" + identifier + "]",
				std::current_exception());
		}

		if (_consumeSnapshotRequest(identifier))
		{
			_publishSnapshot(identifier, _buildRenderSnapshot(identifier, state));
		}
	}

	void Application::UpdateRuntime::finishCycle()
	{
		_lastTime = _currentTime;
	}

	void Application::UpdateRuntime::release(Window::State &state)
	{
		if (state.lifeCycle() == Window::LifeCycle::Released)
		{
			return;
		}
		state.beginRelease();
		state.markReleased();
	}
}
