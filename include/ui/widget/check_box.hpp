#pragma once

#include <string>
#include <string_view>

#include "design_pattern/contract_provider.hpp"
#include "ui/layout/linear_layout.hpp"
#include "ui/widget/checkable_icon_button.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class CheckBox : public Widget
	{
	public:
		using StateProvider = ContractProvider<bool>;
		using StateCallback = StateProvider::callback_type;
		using StateContract = StateProvider::Contract;

	private:
		CheckableIconButton _indicator;
		TextLabel _label;
		HorizontalLayout _layout;
		CheckableIconButton::StateContract _indicatorContract;
		StateProvider _stateProvider;
		unsigned int _spacing = 4;
		Vector2UInt _indicatorSize{16, 16};
		bool _pressed = false;

		void _updateSizeHint() override;
		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;

	public:
		explicit CheckBox(std::string name, Widget *parent = nullptr);
		CheckBox(std::string name, const SpriteSheet *iconset, Font *font, Widget *parent = nullptr);
		CheckBox(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Font *font, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setChecked(bool checked);
		void toggle();
		void setText(Font::Text text);
		void setText(std::string_view text);
		void setSpacing(unsigned int spacing);
		void setIndicatorSize(const Vector2UInt &size);

		[[nodiscard]] bool isChecked() const noexcept;
		[[nodiscard]] unsigned int spacing() const noexcept;
		[[nodiscard]] const Vector2UInt &indicatorSize() const noexcept;
		[[nodiscard]] StateContract subscribeToState(StateCallback callback);
		[[nodiscard]] CheckableIconButton &indicator() noexcept;
		[[nodiscard]] const CheckableIconButton &indicator() const noexcept;
		[[nodiscard]] TextLabel &label() noexcept;
		[[nodiscard]] const TextLabel &label() const noexcept;
	};
}
