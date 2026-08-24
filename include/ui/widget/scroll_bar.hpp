#pragma once

#include <cstddef>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "type/orientation.hpp"
#include "ui/widget/icon_button.hpp"
#include "ui/widget/slider_bar.hpp"

namespace spk
{
	class ScrollBar : public Widget
	{
	public:
		struct ArrowSpriteIDs
		{
			std::size_t up = 4;
			std::size_t down = 5;
			std::size_t left = 6;
			std::size_t right = 7;

			[[nodiscard]] bool operator==(const ArrowSpriteIDs &other) const = default;
		};

		using EditionProvider = ContractProvider<float>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;

	private:
		IconButton _negativeButton;
		SliderBar _slider;
		IconButton _positiveButton;
		IconButton::ClickContract _negativeClickContract;
		SliderBar::EditionContract _sliderEditionContract;
		IconButton::ClickContract _positiveClickContract;
		Orientation _orientation = Orientation::Horizontal;
		float _step = 0.1f;
		ArrowSpriteIDs _arrowSpriteIDs{};
		const SpriteSheet *_iconset = nullptr;
		EditionProvider _editionProvider;

		void _updateIcons();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit ScrollBar(std::string name, Widget *parent = nullptr);
		ScrollBar(std::string name, const SpriteSheet *iconset, Orientation orientation = Orientation::Horizontal, Widget *parent = nullptr);

		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback);

		void setOrientation(Orientation orientation);
		void setStep(float step);
		void setScale(float scale);
		void setRatio(float ratio);
		void setRange(float minimum, float maximum);
		void setValue(float value);
		void setIconset(const SpriteSheet *iconset);
		void setArrowSpriteIDs(const ArrowSpriteIDs &spriteIDs);

		[[nodiscard]] Orientation orientation() const noexcept;
		[[nodiscard]] float step() const noexcept;
		[[nodiscard]] float scale() const noexcept;
		[[nodiscard]] float ratio() const noexcept;
		[[nodiscard]] float value() const noexcept;
		[[nodiscard]] const SliderBar::Range &range() const noexcept;
		[[nodiscard]] const SpriteSheet *iconset() const noexcept;
		[[nodiscard]] const ArrowSpriteIDs &arrowSpriteIDs() const noexcept;

		[[nodiscard]] IconButton &negativeButton() noexcept;
		[[nodiscard]] const IconButton &negativeButton() const noexcept;
		[[nodiscard]] SliderBar &slider() noexcept;
		[[nodiscard]] const SliderBar &slider() const noexcept;
		[[nodiscard]] IconButton &positiveButton() noexcept;
		[[nodiscard]] const IconButton &positiveButton() const noexcept;
	};
}
