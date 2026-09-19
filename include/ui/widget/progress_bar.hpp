#pragma once

#include <functional>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class ProgressBar : public Widget
	{
	public:
		enum class FillDirection
		{
			LeftToRight,
			RightToLeft,
			BottomToTop,
			TopToBottom
		};

		using EditionProvider = ContractProvider<float>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;

	private:
		Panel _fill;
		float _ratio = 0.0f;
		FillDirection _fillDirection = FillDirection::LeftToRight;
		EditionProvider _editionProvider;

		void _updateFillGeometry();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit ProgressBar(std::string name, Widget *parent = nullptr);
		ProgressBar(std::string name, const SpriteSheet *fillTexture, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setRatio(float ratio);
		void setFillDirection(FillDirection direction);
		void setFillTexture(const SpriteSheet *fillTexture);
		void setCornerSize(const Vector2Int &cornerSize);
		void setFillDepth(float depth);

		[[nodiscard]] float ratio() const noexcept;
		[[nodiscard]] FillDirection fillDirection() const noexcept;
		[[nodiscard]] const SpriteSheet *fillTexture() const noexcept;
		[[nodiscard]] const Vector2Int &cornerSize() const noexcept;
		[[nodiscard]] float fillDepth() const noexcept;
		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback);
	};

	class LabeledProgressBar : public Widget
	{
	public:
		using TextPredicate = std::function<std::string(float)>;
		using EditionContract = ProgressBar::EditionContract;
		using EditionCallback = ProgressBar::EditionCallback;

	private:
		ProgressBar _progressBar;
		TextLabel _label;
		TextPredicate _textPredicate;
		ProgressBar::EditionContract _editionContract;

		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit LabeledProgressBar(std::string name, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setRatio(float ratio);
		void setFillDirection(ProgressBar::FillDirection direction);
		void setFillTexture(const SpriteSheet *fillTexture);
		void setCornerSize(const Vector2Int &cornerSize);
		void setFillDepth(float depth);
		void setTextPredicate(TextPredicate predicate);
		void refreshText();

		[[nodiscard]] float ratio() const noexcept;
		[[nodiscard]] ProgressBar::FillDirection fillDirection() const noexcept;
		[[nodiscard]] const SpriteSheet *fillTexture() const noexcept;
		[[nodiscard]] const Vector2Int &cornerSize() const noexcept;
		[[nodiscard]] float fillDepth() const noexcept;
		[[nodiscard]] const TextPredicate &textPredicate() const noexcept;
		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback);
		[[nodiscard]] TextLabel &label() noexcept;
		[[nodiscard]] const TextLabel &label() const noexcept;
	};
}
