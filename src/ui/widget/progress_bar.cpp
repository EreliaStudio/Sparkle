#include "ui/widget/progress_bar.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace spk
{
	ProgressBar::ProgressBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_fill(this->name() + ".fill", this)
	{
		applyStyle(defaultStyle);
		_fill.setMaximalSize({std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});
		_updateSizeHint();
		activate();
	}

	void ProgressBar::applyStyle(const Style &style)
	{
		if (style.sliderBody != nullptr)
		{
			_fill.setSpriteSheet(style.sliderBody.get());
		}
		_fill.setCornerSize({7, 7});
		_updateSizeHint();
	}

	ProgressBar::ProgressBar(std::string name, const SpriteSheet *fillTexture, Widget *parent) :
		ProgressBar(std::move(name), parent)
	{
		setFillTexture(fillTexture);
	}

	void ProgressBar::_updateFillGeometry()
	{
		const unsigned int width = static_cast<unsigned int>(std::lround(static_cast<float>(geometry().width) * _ratio));
		const unsigned int height = static_cast<unsigned int>(std::lround(static_cast<float>(geometry().height) * _ratio));
		Rect2D fill{Vector2Int{0, 0}, geometry().size};
		switch (_fillDirection)
		{
		case FillDirection::LeftToRight:
			fill.width = width;
			break;
		case FillDirection::RightToLeft:
			fill.x = static_cast<int>(geometry().width - width);
			fill.width = width;
			break;
		case FillDirection::BottomToTop:
			fill.y = static_cast<int>(geometry().height - height);
			fill.height = height;
			break;
		case FillDirection::TopToBottom:
			fill.height = height;
			break;
		}
		_fill.setGeometry(fill);
	}

	void ProgressBar::_updateSizeHint()
	{
		setSizeHint(_fill.sizeHint());
	}

	void ProgressBar::_onGeometryChange()
	{
		_updateFillGeometry();
	}

	void ProgressBar::setRatio(float ratio)
	{
		ratio = std::clamp(ratio, 0.0f, 1.0f);
		if (_ratio == ratio)
		{
			return;
		}
		_ratio = ratio;
		_updateFillGeometry();
		_editionProvider.trigger(_ratio);
	}

	void ProgressBar::setFillDirection(FillDirection direction)
	{
		if (_fillDirection != direction)
		{
			_fillDirection = direction;
			_updateFillGeometry();
		}
	}

	void ProgressBar::setFillTexture(const SpriteSheet *fillTexture)
	{
		_fill.setSpriteSheet(fillTexture);
	}
	void ProgressBar::setCornerSize(const Vector2Int &cornerSize)
	{
		_fill.setCornerSize(cornerSize);
	}
	void ProgressBar::setFillDepth(float depth)
	{
		_fill.setDepth(depth);
	}
	float ProgressBar::ratio() const noexcept
	{
		return _ratio;
	}
	ProgressBar::FillDirection ProgressBar::fillDirection() const noexcept
	{
		return _fillDirection;
	}
	const SpriteSheet *ProgressBar::fillTexture() const noexcept
	{
		return _fill.spriteSheet();
	}
	const Vector2Int &ProgressBar::cornerSize() const noexcept
	{
		return _fill.cornerSize();
	}
	float ProgressBar::fillDepth() const noexcept
	{
		return _fill.depth();
	}
	ProgressBar::EditionContract ProgressBar::subscribeToEdition(EditionCallback callback)
	{
		return _editionProvider.subscribe(std::move(callback));
	}

	LabeledProgressBar::LabeledProgressBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_progressBar(this->name() + ".progress", this),
		_label(this->name() + ".label", this),
		_textPredicate([](float ratio) {
			return std::to_string(static_cast<int>(std::lround(ratio * 100.0f))) + "%";
		})
	{
		applyStyle(defaultStyle);
		_label.setMaximalSize({std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});
		_progressBar.setZOrder(0.0f);
		_label.setZOrder(1.0f);
		_editionContract = _progressBar.subscribeToEdition([this](float) {
			refreshText();
		});
		refreshText();
		_updateSizeHint();
		activate();
	}

	void LabeledProgressBar::applyStyle(const Style &style)
	{
		_progressBar.applyStyle(style);
		_label.applyStyle(style);
		_updateSizeHint();
	}

	void LabeledProgressBar::_updateSizeHint()
	{
		const auto maximum = [](const Vector2 &lhs, const Vector2 &rhs) {
			return Vector2{std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
		};
		setSizeHint({maximum(_progressBar.minimalSize(), _label.minimalSize()), maximum(_progressBar.maximalSize(), _label.maximalSize()), maximum(_progressBar.preferredSize(), _label.preferredSize())});
	}

	void LabeledProgressBar::_onGeometryChange()
	{
		const Rect2D fill{Vector2Int{0, 0}, geometry().size};
		_progressBar.setGeometry(fill);
		_label.setGeometry(fill);
	}

	void LabeledProgressBar::setRatio(float ratio)
	{
		_progressBar.setRatio(ratio);
	}
	void LabeledProgressBar::setFillDirection(ProgressBar::FillDirection direction)
	{
		_progressBar.setFillDirection(direction);
	}
	void LabeledProgressBar::setFillTexture(const SpriteSheet *texture)
	{
		_progressBar.setFillTexture(texture);
	}
	void LabeledProgressBar::setCornerSize(const Vector2Int &size)
	{
		_progressBar.setCornerSize(size);
	}
	void LabeledProgressBar::setFillDepth(float depth)
	{
		_progressBar.setFillDepth(depth);
	}
	void LabeledProgressBar::setTextPredicate(TextPredicate predicate)
	{
		_textPredicate = std::move(predicate);
		refreshText();
	}
	void LabeledProgressBar::refreshText()
	{
		_label.setText(_textPredicate ? _textPredicate(ratio()) : std::string{});
	}
	float LabeledProgressBar::ratio() const noexcept
	{
		return _progressBar.ratio();
	}
	ProgressBar::FillDirection LabeledProgressBar::fillDirection() const noexcept
	{
		return _progressBar.fillDirection();
	}
	const SpriteSheet *LabeledProgressBar::fillTexture() const noexcept
	{
		return _progressBar.fillTexture();
	}
	const Vector2Int &LabeledProgressBar::cornerSize() const noexcept
	{
		return _progressBar.cornerSize();
	}
	float LabeledProgressBar::fillDepth() const noexcept
	{
		return _progressBar.fillDepth();
	}
	const LabeledProgressBar::TextPredicate &LabeledProgressBar::textPredicate() const noexcept
	{
		return _textPredicate;
	}
	LabeledProgressBar::EditionContract LabeledProgressBar::subscribeToEdition(EditionCallback callback)
	{
		return _progressBar.subscribeToEdition(std::move(callback));
	}
	TextLabel &LabeledProgressBar::label() noexcept
	{
		return _label;
	}
	const TextLabel &LabeledProgressBar::label() const noexcept
	{
		return _label;
	}
}
