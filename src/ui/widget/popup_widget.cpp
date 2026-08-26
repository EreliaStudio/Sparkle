#include "ui/widget/popup_widget.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{
	std::unordered_map<spk::Widget *, spk::PopupWidget *> openPopups;
}

namespace spk
{
	PopupWidget::PopupWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		setTargetRenderPass(PopupKey);
		setZOrder(1000000.0f);
		deactivate();
	}
	PopupWidget::~PopupWidget()
	{
		auto it = openPopups.find(_coordinatorRoot);
		if (it != openPopups.end() && it->second == this)
		{
			openPopups.erase(it);
		}
	}
	Rect2D PopupWidget::_placedGeometry(const Vector2Int *explicitPosition) const
	{
		const Widget &rootWidget = root();
		const Rect2D rootBounds{Vector2Int{0, 0}, rootWidget.geometry().size};
		Vector2UInt size = geometry().size;
		if (_content != nullptr)
		{
			size.x = std::max(size.x, static_cast<unsigned int>(std::max(_content->preferredSize().x, 0.0f)));
			size.y = std::max(size.y, static_cast<unsigned int>(std::max(_content->preferredSize().y, 0.0f)));
		}
		Vector2Int position = explicitPosition != nullptr ? *explicitPosition : Vector2Int{};
		if (explicitPosition == nullptr)
		{
			if (_anchorWidget == nullptr)
			{
				throw std::logic_error("PopupWidget requires an anchor before open()");
			}
			const Rect2D anchor = _anchorWidget->viewRegion().viewport;
			const Vector2Int rootOrigin = rootWidget.viewRegion().viewport.anchor;
			const int left = anchor.x - rootOrigin.x;
			const int top = anchor.y - rootOrigin.y;
			switch (_placement.horizontal)
			{
			case Alignment::Horizontal::Left:
				position.x = left - static_cast<int>(size.x);
				break;
			case Alignment::Horizontal::Center:
				position.x = left + (static_cast<int>(anchor.width) - static_cast<int>(size.x)) / 2;
				break;
			case Alignment::Horizontal::Right:
				position.x = left + static_cast<int>(anchor.width);
				break;
			}
			switch (_placement.vertical)
			{
			case Alignment::Vertical::Top:
				position.y = top - static_cast<int>(size.y);
				break;
			case Alignment::Vertical::Center:
				position.y = top + (static_cast<int>(anchor.height) - static_cast<int>(size.y)) / 2;
				break;
			case Alignment::Vertical::Bottom:
				position.y = top + static_cast<int>(anchor.height);
				break;
			}
		}
		position += _offset;
		if (_constrainToRoot)
		{
			size.x = std::min(size.x, rootBounds.width);
			size.y = std::min(size.y, rootBounds.height);
			position.x = std::clamp(position.x, 0, static_cast<int>(rootBounds.width - size.x));
			position.y = std::clamp(position.y, 0, static_cast<int>(rootBounds.height - size.y));
		}
		return {position, size};
	}
	void PopupWidget::_onGeometryChange()
	{
		if (_content != nullptr)
		{
			_content->setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
		}
	}
	void PopupWidget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (_open && _closeOnOutsidePress && !viewRegion().viewport.contains(event.device.position))
		{
			close();
			event.consumed = true;
		}
	}
	void PopupWidget::_onKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_open && _closeOnEscape && event.record.key == Keyboard::Escape)
		{
			close();
			event.consumed = true;
		}
	}
	void PopupWidget::_onPassiveKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_open && _closeOnEscape && event.record.key == Keyboard::Escape)
		{
			close();
			event.consumed = true;
		}
	}
	void PopupWidget::setContent(Widget *content)
	{
		if (content != nullptr && content->parent() != this)
		{
			throw std::invalid_argument("PopupWidget content must already be a child of the popup");
		}
		if (_content == content)
		{
			return;
		}
		_contentParentContract.resign();
		_content = content;
		if (_content != nullptr)
		{
			Widget *observed = _content;
			_contentParentContract = observed->subscribeToParentEdition([this, observed](const Widget *parent) {
				if (_content == observed && parent != this)
				{
					_content = nullptr;
					_contentParentContract.resign();
				}
			});
		}
		_onGeometryChange();
	}
	void PopupWidget::setAnchorWidget(Widget *anchor)
	{
		_anchorWidget = anchor;
	}
	void PopupWidget::setPlacement(Alignment placement)
	{
		_placement = placement;
	}
	void PopupWidget::setOffset(const Vector2Int &offset)
	{
		_offset = offset;
	}
	void PopupWidget::setCloseOnOutsidePress(bool enabled)
	{
		_closeOnOutsidePress = enabled;
	}
	void PopupWidget::setCloseOnEscape(bool enabled)
	{
		_closeOnEscape = enabled;
	}
	void PopupWidget::setConstrainToRoot(bool enabled)
	{
		_constrainToRoot = enabled;
	}
	void PopupWidget::open()
	{
		Widget &rootWidget = root();
		Rect2D target = _placedGeometry();
		if (auto it = openPopups.find(&rootWidget); it != openPopups.end() && it->second != this)
		{
			it->second->close();
		}
		if (parent() != &rootWidget)
		{
			setParent(&rootWidget);
		}
		setGeometry(target);
		_open = true;
		_coordinatorRoot = &rootWidget;
		openPopups[&rootWidget] = this;
		activate();
	}
	void PopupWidget::openAt(const Vector2Int &position)
	{
		Widget &rootWidget = root();
		Rect2D target = _placedGeometry(&position);
		if (auto it = openPopups.find(&rootWidget); it != openPopups.end() && it->second != this)
		{
			it->second->close();
		}
		if (parent() != &rootWidget)
		{
			setParent(&rootWidget);
		}
		setGeometry(target);
		_open = true;
		_coordinatorRoot = &rootWidget;
		openPopups[&rootWidget] = this;
		activate();
	}
	void PopupWidget::close()
	{
		if (!_open)
		{
			return;
		}
		_open = false;
		auto it = openPopups.find(_coordinatorRoot);
		if (it != openPopups.end() && it->second == this)
		{
			openPopups.erase(it);
		}
		_coordinatorRoot = nullptr;
		deactivate();
		_closeProvider.trigger();
	}
	bool PopupWidget::isOpen() const noexcept
	{
		return _open;
	}
	Widget *PopupWidget::content() noexcept
	{
		return _content;
	}
	const Widget *PopupWidget::content() const noexcept
	{
		return _content;
	}
	Widget *PopupWidget::anchorWidget() noexcept
	{
		return _anchorWidget;
	}
	const Widget *PopupWidget::anchorWidget() const noexcept
	{
		return _anchorWidget;
	}
	Alignment PopupWidget::placement() const noexcept
	{
		return _placement;
	}
	PopupWidget::CloseContract PopupWidget::subscribeToClose(CloseCallback callback)
	{
		return _closeProvider.subscribe(std::move(callback));
	}
}
