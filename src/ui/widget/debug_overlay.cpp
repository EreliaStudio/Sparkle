#include "ui/widget/debug_overlay.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	DebugOverlay::DebugOverlay(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		_layout.setElementPadding({4, 4});
		_layoutSizeContract = _layout.subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_updateSizeHint();
		});
		_layoutReady = true;
		_updateSizeHint();
		activate();
	}

	DebugOverlay::~DebugOverlay()
	{
		_widgetParentContracts.clear();
		_layoutSizeContract.resign();
		_layout.clear();
	}

	bool DebugOverlay::_containsWidget(const Widget *widget) const noexcept
	{
		for (std::size_t row = 0; row < _layout.rowCount(); ++row)
		{
			for (std::size_t column = 0; column < _layout.columnCount(); ++column)
			{
				const Layout::Element *element = _layout.element(column, row);
				if (element != nullptr && element->widget() == widget)
				{
					return true;
				}
			}
		}
		return false;
	}

	void DebugOverlay::_forgetWidget(Widget *widget)
	{
		for (std::size_t row = 0; row < _layout.rowCount(); ++row)
		{
			for (std::size_t column = 0; column < _layout.columnCount(); ++column)
			{
				Layout::Element *element = _layout.element(column, row);
				if (element != nullptr && element->widget() == widget)
				{
					_layout.clearCell(column, row);
					_widgetParentContracts.erase(widget);
					_onGeometryChange();
					return;
				}
			}
		}
		_widgetParentContracts.erase(widget);
	}

	void DebugOverlay::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}
		setSizeHint(_layout.sizeHint());
		_onGeometryChange();
	}

	void DebugOverlay::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}
		_layout.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
	}

	Layout::Element *DebugOverlay::setWidget(
		std::size_t column,
		std::size_t row,
		Widget *widget,
		Layout::SizeSettings sizeSettings)
	{
		if (widget == nullptr)
		{
			throw std::invalid_argument("DebugOverlay cannot contain a null widget");
		}
		if (widget->parent() != this)
		{
			throw std::invalid_argument("DebugOverlay widgets must already be children of the overlay");
		}

		Layout::Element *currentElement = _layout.element(column, row);
		if (currentElement != nullptr && currentElement->widget() == widget)
		{
			currentElement->setSizeSettings(sizeSettings);
			_onGeometryChange();
			return currentElement;
		}
		if (_containsWidget(widget))
		{
			throw std::invalid_argument("DebugOverlay cannot place the same widget in multiple cells");
		}

		Widget *replacedWidget = currentElement == nullptr ? nullptr : currentElement->widget();
		Layout::Element *result = _layout.setWidget(column, row, widget, sizeSettings);
		if (replacedWidget != nullptr)
		{
			_widgetParentContracts.erase(replacedWidget);
		}

		Widget *observedWidget = widget;
		_widgetParentContracts.emplace(
			widget,
			widget->subscribeToParentEdition([this, observedWidget](const Widget *newParent) {
				if (newParent != this)
				{
					_forgetWidget(observedWidget);
				}
			}));
		_onGeometryChange();
		return result;
	}

	void DebugOverlay::clearCell(std::size_t column, std::size_t row)
	{
		Layout::Element *element = _layout.element(column, row);
		if (element == nullptr)
		{
			return;
		}
		Widget *removedWidget = element->widget();
		_layout.clearCell(column, row);
		if (removedWidget != nullptr)
		{
			_widgetParentContracts.erase(removedWidget);
		}
		_onGeometryChange();
	}

	void DebugOverlay::clear()
	{
		_widgetParentContracts.clear();
		_layout.clear();
		_onGeometryChange();
	}

	void DebugOverlay::setElementPadding(const Vector2UInt &padding)
	{
		_layout.setElementPadding(padding);
		_onGeometryChange();
	}

	const Vector2UInt &DebugOverlay::elementPadding() const noexcept
	{
		return _layout.elementPadding();
	}

	std::size_t DebugOverlay::rowCount() const noexcept
	{
		return _layout.rowCount();
	}

	std::size_t DebugOverlay::columnCount() const noexcept
	{
		return _layout.columnCount();
	}

	Widget *DebugOverlay::widget(std::size_t column, std::size_t row) noexcept
	{
		Layout::Element *element = _layout.element(column, row);
		return element == nullptr ? nullptr : element->widget();
	}

	const Widget *DebugOverlay::widget(std::size_t column, std::size_t row) const noexcept
	{
		const Layout::Element *element = _layout.element(column, row);
		return element == nullptr ? nullptr : element->widget();
	}
}
