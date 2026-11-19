#pragma once

#include <cstdint>
#include <string>

#include "container/flags.hpp"

#include "input/device_context.hpp"
#include "rendering/engine/engine.hpp"
#include "ui/widget.hpp"

namespace spk
{
	/**
	 * @brief Widget that forwards updates and interaction events to a graphical Engine.
	 *
	 * The engine is not owned by the widget and must outlive it, or be detached
	 * with setEngine(nullptr) before it is destroyed.
	 */
	class EngineWidget : public Widget
	{
	public:
		enum class Type : std::uint8_t
		{
			None = 0,
			Update = 1 << 0,
			Render = 1 << 1,
			Draw = 1 << 2
		};

		static constexpr Flags<Type> DefaultType = Flags<Type>(Type::Update) | Type::Render | Type::Draw;

	private:
		Engine *_engine = nullptr;
		Flags<Type> _type;
		std::string _viewName = "main";

		void _onGeometryChange() override;
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override;
		void _updateState(UpdateContext &context) override;
		void _updateState(UpdateContext &context, DeviceContext &deviceContext) override;

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
		EngineWidget(std::string name, Widget *parent, Flags<Type> type = DefaultType);

		[[nodiscard]] Flags<Type> type() const noexcept;
		void setType(Flags<Type> type);
		void setViewName(std::string name);
		[[nodiscard]] const std::string &viewName() const noexcept;

		void setEngine(Engine *engine) noexcept;
		[[nodiscard]] Engine *engine() noexcept;
		[[nodiscard]] const Engine *engine() const noexcept;
	};

	[[nodiscard]] constexpr Flags<EngineWidget::Type> operator|(
		EngineWidget::Type lhs, EngineWidget::Type rhs) noexcept
	{
		return Flags<EngineWidget::Type>(lhs) | rhs;
	}
}
