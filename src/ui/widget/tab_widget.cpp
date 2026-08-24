#include "ui/widget/tab_widget.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace spk
{
	TabWidget::TabWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		activate();
	}

	void TabWidget::_applySelection()
	{
		for (std::size_t index = 0; index < _pages.size(); ++index)
		{
			_pages[index].button->setFlat(!_selectedIndex.has_value() || index != *_selectedIndex);
			if (_selectedIndex.has_value() && index == *_selectedIndex)
			{
				_pages[index].content->activate();
			}
			else
			{
				_pages[index].content->deactivate();
			}
		}
	}

	void TabWidget::_updateSizeHint()
	{
		Vector2 contentMinimum{};
		Vector2 contentPreferred{};
		for (const Page &page : _pages)
		{
			contentMinimum.x = std::max(contentMinimum.x, page.content->minimalSize().x);
			contentMinimum.y = std::max(contentMinimum.y, page.content->minimalSize().y);
			contentPreferred.x = std::max(contentPreferred.x, page.content->preferredSize().x);
			contentPreferred.y = std::max(contentPreferred.y, page.content->preferredSize().y);
		}

		const float tabsWidth = static_cast<float>(_tabWidth) * static_cast<float>(_pages.size());
		SizeHint hint = sizeHint();
		hint.minimal = {std::max(contentMinimum.x, tabsWidth), contentMinimum.y + static_cast<float>(_tabBarHeight)};
		hint.preferred = {std::max(contentPreferred.x, tabsWidth), contentPreferred.y + static_cast<float>(_tabBarHeight)};
		setSizeHint(hint);
	}

	void TabWidget::_onGeometryChange()
	{
		const unsigned int barHeight = std::min(_tabBarHeight, geometry().height);
		const unsigned int contentHeight = geometry().height - barHeight;
		const unsigned int buttonWidth = _pages.empty() ? 0 : std::min(_tabWidth, geometry().width / static_cast<unsigned int>(_pages.size()));
		for (std::size_t index = 0; index < _pages.size(); ++index)
		{
			_pages[index].button->setGeometry({Vector2Int{static_cast<int>(index * buttonWidth), 0}, Vector2UInt{buttonWidth, barHeight}});
			_pages[index].content->setGeometry({Vector2Int{0, static_cast<int>(barHeight)}, Vector2UInt{geometry().width, contentHeight}});
		}
	}

	PushButton &TabWidget::addPage(std::string_view title, Widget *content)
	{
		if (content == nullptr)
		{
			throw std::invalid_argument("TabWidget page cannot be null");
		}
		if (content->parent() != this)
		{
			throw std::invalid_argument("TabWidget page must already be a child of the tab widget");
		}
		if (std::ranges::any_of(_pages, [content](const Page &page) {
				return page.content == content;
			}))
		{
			throw std::invalid_argument("TabWidget cannot register the same page twice");
		}

		auto button = std::make_unique<PushButton>(name() + ".tab-" + std::to_string(_pages.size()), this);
		button->setText(title);
		button->setZOrder(1.0f);
		content->setZOrder(0.0f);
		PushButton *buttonPointer = button.get();
		auto clickContract = button->subscribeToClick([this, content]() {
			const auto it = std::ranges::find_if(_pages, [content](const Page &page) {
				return page.content == content;
			});
			if (it != _pages.end())
			{
				selectPage(static_cast<std::size_t>(std::distance(_pages.begin(), it)));
			}
		});
		_pages.push_back(Page{content, std::move(button), std::move(clickContract)});
		if (!_selectedIndex.has_value())
		{
			_selectedIndex = 0;
		}
		_applySelection();
		_updateSizeHint();
		_onGeometryChange();
		return *buttonPointer;
	}

	void TabWidget::removePage(Widget *content)
	{
		const auto it = std::ranges::find_if(_pages, [content](const Page &page) {
			return page.content == content;
		});
		if (it == _pages.end())
		{
			return;
		}
		const std::size_t removedIndex = static_cast<std::size_t>(std::distance(_pages.begin(), it));
		content->deactivate();
		_pages.erase(it);
		if (_pages.empty())
		{
			_selectedIndex.reset();
		}
		else if (_selectedIndex.has_value())
		{
			if (*_selectedIndex > removedIndex)
			{
				--*_selectedIndex;
			}
			else if (*_selectedIndex == removedIndex)
			{
				*_selectedIndex = std::min(removedIndex, _pages.size() - 1);
				_selectionProvider.trigger(*_selectedIndex);
			}
		}
		_applySelection();
		_updateSizeHint();
		_onGeometryChange();
	}

	void TabWidget::selectPage(std::size_t index)
	{
		if (index >= _pages.size())
		{
			throw std::out_of_range("TabWidget page index is out of range");
		}
		if (_selectedIndex.has_value() && *_selectedIndex == index)
		{
			return;
		}
		_selectedIndex = index;
		_applySelection();
		_selectionProvider.trigger(index);
	}

	void TabWidget::setTabBarHeight(unsigned int height)
	{
		if (_tabBarHeight == height)
		{
			return;
		}
		_tabBarHeight = height;
		_updateSizeHint();
		_onGeometryChange();
	}

	void TabWidget::setTabWidth(unsigned int width)
	{
		if (_tabWidth == width)
		{
			return;
		}
		_tabWidth = width;
		_updateSizeHint();
		_onGeometryChange();
	}

	TabWidget::SelectionContract TabWidget::subscribeToSelection(SelectionCallback callback)
	{
		return _selectionProvider.subscribe(std::move(callback));
	}

	std::size_t TabWidget::pageCount() const noexcept
	{
		return _pages.size();
	}

	std::optional<std::size_t> TabWidget::selectedIndex() const noexcept
	{
		return _selectedIndex;
	}

	Widget *TabWidget::selectedPage() noexcept
	{
		return _selectedIndex.has_value() ? _pages[*_selectedIndex].content : nullptr;
	}

	const Widget *TabWidget::selectedPage() const noexcept
	{
		return _selectedIndex.has_value() ? _pages[*_selectedIndex].content : nullptr;
	}

	Widget &TabWidget::page(std::size_t index)
	{
		return *_pages.at(index).content;
	}

	const Widget &TabWidget::page(std::size_t index) const
	{
		return *_pages.at(index).content;
	}

	PushButton &TabWidget::tabButton(std::size_t index)
	{
		return *_pages.at(index).button;
	}

	const PushButton &TabWidget::tabButton(std::size_t index) const
	{
		return *_pages.at(index).button;
	}

	unsigned int TabWidget::tabBarHeight() const noexcept
	{
		return _tabBarHeight;
	}

	unsigned int TabWidget::tabWidth() const noexcept
	{
		return _tabWidth;
	}
}
