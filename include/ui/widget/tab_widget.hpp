#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/push_button.hpp"

namespace spk
{
	class TabWidget : public Widget
	{
	public:
		using SelectionProvider = ContractProvider<std::size_t>;
		using SelectionCallback = SelectionProvider::callback_type;
		using SelectionContract = SelectionProvider::Contract;

	private:
		struct Page
		{
			Widget *content = nullptr;
			std::unique_ptr<PushButton> button;
			PushButton::ClickContract clickContract;
		};

		std::vector<Page> _pages;
		std::optional<std::size_t> _selectedIndex;
		unsigned int _tabBarHeight = 40;
		unsigned int _tabWidth = 160;
		SelectionProvider _selectionProvider;

		void _applySelection();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit TabWidget(std::string name, Widget *parent = nullptr);

		PushButton &addPage(std::string_view title, Widget *content);
		void removePage(Widget *content);
		void selectPage(std::size_t index);
		void setTabBarHeight(unsigned int height);
		void setTabWidth(unsigned int width);

		[[nodiscard]] SelectionContract subscribeToSelection(SelectionCallback callback);
		[[nodiscard]] std::size_t pageCount() const noexcept;
		[[nodiscard]] std::optional<std::size_t> selectedIndex() const noexcept;
		[[nodiscard]] Widget *selectedPage() noexcept;
		[[nodiscard]] const Widget *selectedPage() const noexcept;
		[[nodiscard]] Widget &page(std::size_t index);
		[[nodiscard]] const Widget &page(std::size_t index) const;
		[[nodiscard]] PushButton &tabButton(std::size_t index);
		[[nodiscard]] const PushButton &tabButton(std::size_t index) const;
		[[nodiscard]] unsigned int tabBarHeight() const noexcept;
		[[nodiscard]] unsigned int tabWidth() const noexcept;
	};
}
