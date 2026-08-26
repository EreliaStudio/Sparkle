#include "ui/widget/tooltip.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "core/context/update_context.hpp"

namespace
{
	std::unordered_map<spk::Widget *, spk::Tooltip *> shownTooltips;
}

namespace spk
{
	Tooltip::Tooltip(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this),
		_textArea(this->name() + ".text", this)
	{
		applyStyle(defaultStyle);
		setTargetRenderPass(TooltipKey);
		setZOrder(2000000.0f);
		_background.setZOrder(0);
		_textArea.setZOrder(1);
		_background.deactivate();
		_textArea.deactivate();
		activate();
	}

	void Tooltip::applyStyle(const Style &style)
	{
		if (style.darkNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkNineSlice.get());
		}
		_background.setCornerSize({9, 9});
		_textArea.applyStyle(style);
	}

	Tooltip::~Tooltip()
	{
		auto it = shownTooltips.find(_coordinatorRoot);
		if (it != shownTooltips.end() && it->second == this)
		{
			shownTooltips.erase(it);
		}
	}
	void Tooltip::_updateGeometry()
	{
		if (_target == nullptr)
		{
			return;
		}
		Widget &rootWidget = root();
		const Vector2Int cornerSize = _background.cornerSize();
		const Vector2UInt requestedPadding{
			static_cast<unsigned int>(std::max(cornerSize.x, 0)),
			static_cast<unsigned int>(std::max(cornerSize.y, 0))};
		const unsigned int availableWidth = std::min(_maximumWidth, rootWidget.geometry().width);
		const Vector2UInt measurementPadding{
			std::min(requestedPadding.x, availableWidth / 2),
			std::min(requestedPadding.y, rootWidget.geometry().height / 2)};
		const unsigned int textWidth = availableWidth - 2 * measurementPadding.x;
		const Vector2UInt preferred = _textArea.computePreferredSize(textWidth);
		Vector2UInt size{
			std::min(preferred.x + 2 * measurementPadding.x, rootWidget.geometry().width),
			std::min(preferred.y + 2 * measurementPadding.y, rootWidget.geometry().height)};
		const Rect2D targetRect = _target->viewRegion().viewport;
		const Vector2Int origin = rootWidget.viewRegion().viewport.anchor;
		Placement resolved = _placement;
		if (resolved == Placement::Automatic)
		{
			resolved = targetRect.y - origin.y >= static_cast<int>(size.y) ? Placement::AboveTarget : Placement::BelowTarget;
		}
		Vector2Int position;
		switch (resolved)
		{
		case Placement::Cursor:
			position = {_lastCursor.x - origin.x + _cursorOffset.x, _lastCursor.y - origin.y + _cursorOffset.y};
			break;
		case Placement::AboveTarget:
			position = {targetRect.x - origin.x, targetRect.y - origin.y - static_cast<int>(size.y)};
			break;
		case Placement::BelowTarget:
			position = {targetRect.x - origin.x, targetRect.y - origin.y + static_cast<int>(targetRect.height)};
			break;
		case Placement::Automatic:
			break;
		}
		position.x = std::clamp(position.x, 0, static_cast<int>(rootWidget.geometry().width - size.x));
		position.y = std::clamp(position.y, 0, static_cast<int>(rootWidget.geometry().height - size.y));
		setGeometry(Rect2D{position, size});
		_background.setGeometry(Rect2D{Vector2Int{0, 0}, size});
		const Vector2UInt renderedPadding{
			std::min(requestedPadding.x, size.x / 2),
			std::min(requestedPadding.y, size.y / 2)};
		_textArea.setGeometry(Rect2D{Vector2Int{static_cast<int>(renderedPadding.x), static_cast<int>(renderedPadding.y)}, Vector2UInt{size.x - 2 * renderedPadding.x, size.y - 2 * renderedPadding.y}});
	}
	void Tooltip::_updateState(UpdateContext &context)
	{
		bool targetIsEffectivelyActive = _target != nullptr;
		for (const Widget *widget = _target; targetIsEffectivelyActive && widget != nullptr; widget = widget->parent())
		{
			targetIsEffectivelyActive = widget->isActive();
		}
		if (!targetIsEffectivelyActive || _textArea.text().empty())
		{
			hide();
			return;
		}
		_lastCursor = context.mouse.position;
		const bool hovering = _target->viewRegion().viewport.contains(context.mouse.position);
		if (hovering)
		{
			_leaveElapsed = {};
			if (!_hovering)
			{
				_hoverElapsed = {};
			}
			_hovering = true;
			if (!_shown)
			{
				_hoverElapsed += context.deltaTime;
				if (_hoverElapsed >= _openDelay)
				{
					show();
				}
			}
			else if (_placement == Placement::Cursor)
			{
				_updateGeometry();
			}
		}
		else
		{
			_hovering = false;
			_hoverElapsed = {};
			if (_shown)
			{
				_leaveElapsed += context.deltaTime;
				if (_leaveElapsed >= _closeDelay)
				{
					hide();
				}
			}
		}
	}
	void Tooltip::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		_lastCursor = event.device.position;
	}
	void Tooltip::_onMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
		hide();
	}
	void Tooltip::_onWindowFocusLostEvent(WindowFocusLostEvent &)
	{
		hide();
	}
	void Tooltip::_onPassiveMouseMovedEvent(MouseMovedEvent &event)
	{
		_lastCursor = event.device.position;
	}
	void Tooltip::_onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
		hide();
	}
	void Tooltip::setTarget(Widget *target)
	{
		hide();
		_targetParentContract.resign();
		_target = target;
		if (_target != nullptr)
		{
			Widget &targetRoot = _target->root();
			if (parent() != &targetRoot)
			{
				setParent(&targetRoot);
			}
			Widget *observed = _target;
			_targetParentContract = observed->subscribeToParentEdition([this, observed](const Widget *) {
				if (_target == observed)
				{
					hide();
				}
			});
		}
	}
	void Tooltip::setText(Font::Text text)
	{
		_textArea.setText(std::move(text));
		if (_textArea.text().empty())
		{
			hide();
		}
		else if (_shown)
		{
			_updateGeometry();
		}
	}
	void Tooltip::setText(std::string_view text)
	{
		setText(Font::textFromUTF8(text));
	}
	void Tooltip::setOpenDelay(Duration duration)
	{
		if (duration < Duration::zero())
		{
			throw std::invalid_argument("Tooltip open delay cannot be negative");
		}
		_openDelay = duration;
	}
	void Tooltip::setCloseDelay(Duration duration)
	{
		if (duration < Duration::zero())
		{
			throw std::invalid_argument("Tooltip close delay cannot be negative");
		}
		_closeDelay = duration;
	}
	void Tooltip::setPlacement(Placement placement)
	{
		_placement = placement;
		if (_shown)
		{
			_updateGeometry();
		}
	}
	void Tooltip::setCursorOffset(const Vector2Int &offset)
	{
		_cursorOffset = offset;
		if (_shown)
		{
			_updateGeometry();
		}
	}
	void Tooltip::setMaximumWidth(unsigned int width)
	{
		_maximumWidth = width;
		if (_shown)
		{
			_updateGeometry();
		}
	}
	void Tooltip::show()
	{
		if (_target == nullptr || _textArea.text().empty())
		{
			return;
		}
		Widget &targetRoot = _target->root();
		if (auto it = shownTooltips.find(&targetRoot); it != shownTooltips.end() && it->second != this)
		{
			it->second->hide();
		}
		if (parent() != &targetRoot)
		{
			setParent(&targetRoot);
		}
		_shown = true;
		_coordinatorRoot = &targetRoot;
		shownTooltips[&targetRoot] = this;
		_leaveElapsed = {};
		_background.activate();
		_textArea.activate();
		_updateGeometry();
	}
	void Tooltip::hide()
	{
		if (_shown)
		{
			auto it = shownTooltips.find(_coordinatorRoot);
			if (it != shownTooltips.end() && it->second == this)
			{
				shownTooltips.erase(it);
			}
		}
		_coordinatorRoot = nullptr;
		_shown = false;
		_hoverElapsed = {};
		_leaveElapsed = {};
		_background.deactivate();
		_textArea.deactivate();
	}
	bool Tooltip::isShown() const noexcept
	{
		return _shown;
	}
	Widget *Tooltip::target() noexcept
	{
		return _target;
	}
	const Widget *Tooltip::target() const noexcept
	{
		return _target;
	}
	Tooltip::Duration Tooltip::openDelay() const noexcept
	{
		return _openDelay;
	}
	Tooltip::Duration Tooltip::closeDelay() const noexcept
	{
		return _closeDelay;
	}
	Panel &Tooltip::background() noexcept
	{
		return _background;
	}
	const Panel &Tooltip::background() const noexcept
	{
		return _background;
	}
	TextArea &Tooltip::textArea() noexcept
	{
		return _textArea;
	}
	const TextArea &Tooltip::textArea() const noexcept
	{
		return _textArea;
	}
}
