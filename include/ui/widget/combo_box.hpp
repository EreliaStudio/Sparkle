#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "ui/text_model.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/popup_widget.hpp"
#include "ui/widget/push_button.hpp"

namespace spk
{
	class ComboBox : public Widget
	{
	public:
		using Model = DataModel<std::string>;
		using ModelView = Model::View;
		using Selection = ModelView::Selection;
		using SelectionProvider = ContractProvider<std::optional<Selection>>;
		using SelectionCallback = SelectionProvider::callback_type;
		using SelectionContract = SelectionProvider::Contract;

	private:
		Panel _background;
		TextLabel _displayLabel;
		PushButton _openButton;
		PopupWidget _popup;
		Panel _popupBackground;
		ModelView _view;
		TextModel::Delegate _defaultDelegate;
		std::unique_ptr<Widget> _displayItem;
		Font::Text _placeholder;
		std::size_t _maximumVisibleRows = 0;
		std::size_t _nextDisplayItemIdentifier = 0;
		unsigned int _openButtonWidth = 32;
		PushButton::ClickContract _openContract;
		ModelView::SelectionContract _viewSelectionContract;
		Model::RowsContract _modelInsertedContract;
		Model::RowsContract _modelRemovedContract;
		Model::RowsContract _modelChangedContract;
		Model::ResetContract _modelResetContract;
		SelectionProvider _selectionProvider;

		void _detachModelContracts();
		void _recreateDisplayItem();
		void _refreshDisplay();
		void _updatePopupContentGeometry();
		void _updatePopupSize();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit ComboBox(std::string name, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;
		void setModel(Model *model);
		void setItemDelegate(ModelView::Delegate *delegate);
		void setSelectedRow(std::optional<std::size_t> row);
		void clearSelection();
		void open();
		void close();
		void setPlaceholder(Font::Text placeholder);
		void setPlaceholder(std::string_view placeholder);
		void setMaximumVisibleRows(std::size_t count);
		void setOpenButtonWidth(unsigned int width);
		[[nodiscard]] bool isOpen() const noexcept;
		[[nodiscard]] Model *model() const noexcept;
		[[nodiscard]] std::optional<std::size_t> selectedRow() const noexcept;
		[[nodiscard]] std::optional<Model::RowID> selectedRowID() const noexcept;
		[[nodiscard]] Widget *selectedWidget() noexcept;
		[[nodiscard]] const Widget *selectedWidget() const noexcept;
		[[nodiscard]] std::optional<Selection> selection() const noexcept;
		[[nodiscard]] const Font::Text &placeholder() const noexcept;
		[[nodiscard]] std::size_t maximumVisibleRows() const noexcept;
		[[nodiscard]] unsigned int openButtonWidth() const noexcept;
		[[nodiscard]] Panel &background() noexcept;
		[[nodiscard]] const Panel &background() const noexcept;
		[[nodiscard]] TextLabel &displayLabel() noexcept;
		[[nodiscard]] const TextLabel &displayLabel() const noexcept;
		[[nodiscard]] Widget *displayItem() noexcept;
		[[nodiscard]] const Widget *displayItem() const noexcept;
		[[nodiscard]] PushButton &openButton() noexcept;
		[[nodiscard]] const PushButton &openButton() const noexcept;
		[[nodiscard]] ModelView &view() noexcept;
		[[nodiscard]] const ModelView &view() const noexcept;
		[[nodiscard]] Panel &popupBackground() noexcept;
		[[nodiscard]] const Panel &popupBackground() const noexcept;
		[[nodiscard]] PopupWidget &popup() noexcept;
		[[nodiscard]] const PopupWidget &popup() const noexcept;
		[[nodiscard]] SelectionContract subscribeToSelection(SelectionCallback callback);
	};
}
