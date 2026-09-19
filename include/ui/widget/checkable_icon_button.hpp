#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/icon_button.hpp"

namespace spk
{
	class CheckableIconButton : public Widget
	{
	public:
		static constexpr std::size_t DefaultUncheckedSpriteID = 9;
		static constexpr std::size_t DefaultCheckedSpriteID = 8;

		using StateProvider = ContractProvider<bool>;
		using StateCallback = StateProvider::callback_type;
		using StateContract = StateProvider::Contract;

	private:
		bool _checked = false;
		IconButton _uncheckedButton;
		IconButton _checkedButton;
		IconButton::ClickContract _uncheckedClickContract;
		IconButton::ClickContract _checkedClickContract;
		StateProvider _stateProvider;

		void _applyState();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit CheckableIconButton(std::string name, Widget *parent = nullptr);
		CheckableIconButton(std::string name, const SpriteSheet *iconset, Widget *parent = nullptr);
		CheckableIconButton(std::string name, const SpriteSheet *iconset, std::size_t uncheckedSpriteID, std::size_t checkedSpriteID, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setChecked(bool checked);
		void toggle();
		void setUncheckedSpriteID(std::size_t spriteID);
		void setUncheckedSpriteID(const Vector2UInt &coordinates);
		void setCheckedSpriteID(std::size_t spriteID);
		void setCheckedSpriteID(const Vector2UInt &coordinates);

		[[nodiscard]] bool isChecked() const noexcept;
		[[nodiscard]] std::size_t uncheckedSpriteID() const noexcept;
		[[nodiscard]] std::size_t checkedSpriteID() const noexcept;
		[[nodiscard]] IconButton &uncheckedButton() noexcept;
		[[nodiscard]] const IconButton &uncheckedButton() const noexcept;
		[[nodiscard]] IconButton &checkedButton() noexcept;
		[[nodiscard]] const IconButton &checkedButton() const noexcept;

		[[nodiscard]] StateContract subscribeToState(StateCallback callback);
		[[nodiscard]] StateContract addStateCallback(bool targetState, std::function<void()> callback);
	};
}
