#pragma once

#include <algorithm>
#include <concepts>
#include <limits>
#include <string>
#include <utility>

#include "ui/widget/menu_bar.hpp"

namespace spk
{
	template <typename TContent>
	concept WorkspaceContent = std::derived_from<TContent, Widget> && std::constructible_from<TContent, std::string, Widget *>;

	template <WorkspaceContent TContent>
	class Workspace : public Widget
	{
	private:
		bool _layoutReady = false;
		TContent _content;
		MenuBar _menuBar;

		void _updateSizeHint() override
		{
			if (!_layoutReady)
			{
				return;
			}
			SizeHint hint = sizeHint();
			hint.minimal = {
				std::max(_menuBar.minimalSize().x, _content.minimalSize().x),
				static_cast<float>(_menuBar.height()) + _content.minimalSize().y};
			hint.preferred = hint.minimal;
			setSizeHint(hint);
			_onGeometryChange();
		}

		void _onGeometryChange() override
		{
			if (!_layoutReady)
			{
				return;
			}
			const unsigned int barHeight = std::min(_menuBar.height(), geometry().height);
			_content.setGeometry(Rect2D{Vector2Int{0, static_cast<int>(barHeight)}, Vector2UInt{geometry().width, geometry().height - barHeight}});
			_menuBar.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
		}

	public:
		explicit Workspace(std::string name, Widget *parent = nullptr) :
			Widget(std::move(name), parent),
			_content(this->name() + ".content", this),
			_menuBar(this->name() + ".menu-bar", this)
		{
			_content.setZOrder(0.0f);
			_menuBar.setZOrder(1.0f);
			const float unlimited = static_cast<float>(std::numeric_limits<std::size_t>::max());
			setMaximalSize({unlimited, unlimited});
			_layoutReady = true;
			_updateSizeHint();
			activate();
		}

		[[nodiscard]] MenuBar &menuBar() noexcept
		{
			return _menuBar;
		}
		[[nodiscard]] const MenuBar &menuBar() const noexcept
		{
			return _menuBar;
		}
		[[nodiscard]] TContent &content() noexcept
		{
			return _content;
		}
		[[nodiscard]] const TContent &content() const noexcept
		{
			return _content;
		}
	};
}
