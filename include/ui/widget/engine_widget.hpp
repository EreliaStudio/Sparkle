#pragma once

#include <string>

#include "engine/engine.hpp"
#include "ui/widget.hpp"

namespace spk
{
	/**
	 * @brief Widget that forwards update and interaction events to an Engine.
	 *
	 * The engine is not owned by the widget and must outlive it, or be detached
	 * with setEngine(nullptr) before it is destroyed.
	 */
	class EngineWidget : public Widget
	{
	private:
		Engine *_engine = nullptr;

		void _onGeometryChange() override;
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override;
		void _updateState(UpdateContext &context) override;

		void _onWindowResizedEvent(WindowResizedEvent &event) override;
		void _onWindowMovedEvent(WindowMovedEvent &event) override;
		void _onWindowFocusGainedEvent(WindowFocusGainedEvent &event) override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;

		void _onMouseEnteredEvent(MouseEnteredEvent &event) override;
		void _onMouseLeftEvent(MouseLeftEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;
		void _onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event) override;

		void _onKeyPressedEvent(KeyPressedEvent &event) override;
		void _onKeyReleasedEvent(KeyReleasedEvent &event) override;
		void _onTextInputEvent(TextInputEvent &event) override;

		void _onPassiveMouseMovedEvent(MouseMovedEvent &event) override;
		void _onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onPassiveKeyPressedEvent(KeyPressedEvent &event) override;
		void _onPassiveKeyReleasedEvent(KeyReleasedEvent &event) override;

	public:
		EngineWidget(std::string name, Widget *parent);

		void setEngine(Engine *engine) noexcept;
		[[nodiscard]] Engine *engine() noexcept;
		[[nodiscard]] const Engine *engine() const noexcept;
	};
}
