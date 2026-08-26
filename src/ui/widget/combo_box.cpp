#include "ui/widget/combo_box.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	ComboBox::ComboBox(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this),
		_displayLabel(this->name() + ".placeholder", this),
		_openButton(this->name() + ".open-button", this),
		_popup(this->name() + ".popup", this),
		_popupBackground(this->name() + ".popup-background", &_popup),
		_view(this->name() + ".view", &_popupBackground)
	{
		_view.setDelegate(&_defaultDelegate);
		applyStyle(defaultStyle);
		_background.setZOrder(0.0f);
		_displayLabel.setZOrder(1.0f);
		_openButton.setZOrder(2.0f);
		_popupBackground.setZOrder(0.0f);
		_view.setZOrder(1.0f);
		_popupBackground.setCornerSize({4, 4});
		_displayLabel.setAlignment({Alignment::Horizontal::Left, Alignment::Vertical::Center});
		_openButton.setText("v");
		_popup.setContent(&_popupBackground);
		_popup.setAnchorWidget(this);
		_openContract = _openButton.subscribeToClick([this]() {
			isOpen() ? close() : open();
		});
		_viewSelectionContract = _view.subscribeToSelection([this](std::optional<Selection> selected) {
			_refreshDisplay();
			_selectionProvider.trigger(selected);
			if (selected.has_value())
			{
				close();
			}
		});
		_refreshDisplay();
		_updateSizeHint();
		activate();
	}

	void ComboBox::applyStyle(const Style &style)
	{
		if (style.darkNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkNineSlice.get());
		}
		if (style.darkerNineSlice != nullptr)
		{
			_popupBackground.setSpriteSheet(style.darkerNineSlice.get());
		}
		_background.setCornerSize({8, 8});
		_popupBackground.setCornerSize({4, 4});
		_displayLabel.applyStyle(style);
		_openButton.applyStyle(style);
		if (style.font != nullptr)
		{
			_defaultDelegate.setFont(style.font.get());
		}
	}

	void ComboBox::_detachModelContracts()
	{
		_modelInsertedContract.resign();
		_modelRemovedContract.resign();
		_modelChangedContract.resign();
		_modelResetContract.resign();
	}

	void ComboBox::_recreateDisplayItem()
	{
		_displayItem.reset();
		Model *currentModel = _view.model();
		ModelView::Delegate *currentDelegate = _view.delegate();
		const std::optional<std::size_t> row = _view.selectedRow();
		if (currentModel != nullptr &&
			currentDelegate != nullptr &&
			row.has_value() &&
			*row < currentModel->rowCount())
		{
			auto item = currentDelegate->createItem(
				name() + ".display-item-" + std::to_string(_nextDisplayItemIdentifier++),
				this);
			if (item == nullptr || item->parent() != this)
			{
				throw std::invalid_argument("ComboBox delegate display item must be a non-null child of the combo box");
			}
			item->setZOrder(1.0f);
			item->activate();
			currentDelegate->bindItem(*item, *currentModel, *row, true);
			_displayItem = std::move(item);
		}
		_updateSizeHint();
		_onGeometryChange();
	}

	void ComboBox::_refreshDisplay()
	{
		Model *currentModel = _view.model();
		ModelView::Delegate *currentDelegate = _view.delegate();
		const std::optional<std::size_t> row = _view.selectedRow();
		const bool hasDisplayRow = currentModel != nullptr &&
								   currentDelegate != nullptr &&
								   row.has_value() &&
								   *row < currentModel->rowCount();

		if (!hasDisplayRow)
		{
			_displayItem.reset();
			_displayLabel.setText(_placeholder);
			_displayLabel.activate();
			_updateSizeHint();
			_onGeometryChange();
			return;
		}

		if (_displayItem == nullptr)
		{
			_recreateDisplayItem();
		}
		else
		{
			currentDelegate->bindItem(*_displayItem, *currentModel, *row, true);
		}
		_displayLabel.deactivate();
	}

	void ComboBox::_updatePopupContentGeometry()
	{
		const Rect2D panelArea{Vector2Int{0, 0}, _popupBackground.geometry().size};
		_view.setGeometry(panelArea.shrink(_popupBackground.cornerSize()));
	}

	void ComboBox::_updatePopupSize()
	{
		unsigned int viewHeight = static_cast<unsigned int>(std::max(_view.preferredSize().y, 0.0f));
		if (_maximumVisibleRows != 0 && _view.model() != nullptr && _view.model()->rowCount() > _maximumVisibleRows)
		{
			viewHeight = static_cast<unsigned int>(std::lround(
				static_cast<double>(viewHeight) *
				static_cast<double>(_maximumVisibleRows) /
				static_cast<double>(_view.model()->rowCount())));
		}
		unsigned int delegatedWidth = static_cast<unsigned int>(std::max(_view.minimalSize().x, 0.0f));
		if (_displayItem != nullptr)
		{
			delegatedWidth = std::max(delegatedWidth, static_cast<unsigned int>(std::max(_displayItem->minimalSize().x, 0.0f)));
		}
		const Vector2Int cornerSize = _popupBackground.cornerSize();
		const unsigned int horizontalMargin = static_cast<unsigned int>(cornerSize.x * 2);
		const unsigned int verticalMargin = static_cast<unsigned int>(cornerSize.y * 2);
		const Vector2UInt popupSize{
			std::max(geometry().width, delegatedWidth + horizontalMargin),
			viewHeight + verticalMargin};
		_popup.setGeometry(Rect2D{Vector2Int{0, 0}, popupSize});
		_updatePopupContentGeometry();
	}

	void ComboBox::_updateSizeHint()
	{
		const Vector2 displayMinimal = _displayItem != nullptr ? _displayItem->minimalSize() : _displayLabel.minimalSize();
		const Vector2 displayPreferred = _displayItem != nullptr ? _displayItem->preferredSize() : _displayLabel.preferredSize();
		const float buttonMinimalWidth = std::max(static_cast<float>(_openButtonWidth), _openButton.minimalSize().x);
		const float buttonPreferredWidth = std::max(static_cast<float>(_openButtonWidth), _openButton.preferredSize().x);
		const Vector2 minimal{
			std::max(_background.minimalSize().x, displayMinimal.x + buttonMinimalWidth),
			std::max({_background.minimalSize().y, displayMinimal.y, _openButton.minimalSize().y})};
		const Vector2 preferred{
			std::max(_background.preferredSize().x, displayPreferred.x + buttonPreferredWidth),
			std::max({_background.preferredSize().y, displayPreferred.y, _openButton.preferredSize().y})};
		setSizeHint({minimal, {std::numeric_limits<float>::max(), preferred.y}, preferred});
	}

	void ComboBox::_onGeometryChange()
	{
		const Rect2D fill{Vector2Int{0, 0}, geometry().size};
		_background.setGeometry(fill);
		const unsigned int buttonWidth = std::min(
			geometry().width,
			std::max(_openButtonWidth, static_cast<unsigned int>(std::max(_openButton.minimalSize().x, 0.0f))));
		const unsigned int displayWidth = geometry().width - buttonWidth;
		const Rect2D displayGeometry{Vector2Int{0, 0}, Vector2UInt{displayWidth, geometry().height}};
		_displayLabel.setGeometry(displayGeometry);
		if (_displayItem != nullptr)
		{
			_displayItem->setGeometry(displayGeometry);
		}
		_openButton.setGeometry(Rect2D{Vector2Int{static_cast<int>(displayWidth), 0}, Vector2UInt{buttonWidth, geometry().height}});
		_updatePopupSize();
	}

	void ComboBox::setModel(Model *model)
	{
		if (_view.model() == model)
		{
			return;
		}
		_detachModelContracts();
		_view.setModel(model);
		if (model != nullptr)
		{
			_modelInsertedContract = model->subscribeToRowsInserted([this](std::size_t, std::size_t) {
				_refreshDisplay();
				_updatePopupSize();
			});
			_modelRemovedContract = model->subscribeToRowsRemoved([this](std::size_t, std::size_t) {
				_refreshDisplay();
				_updatePopupSize();
			});
			_modelChangedContract = model->subscribeToRowsChanged([this](std::size_t, std::size_t) {
				_refreshDisplay();
				_updatePopupSize();
			});
			_modelResetContract = model->subscribeToReset([this]() {
				_refreshDisplay();
				_updatePopupSize();
			});
		}
		_refreshDisplay();
		_updatePopupSize();
	}

	void ComboBox::setItemDelegate(ModelView::Delegate *delegate)
	{
		_view.setDelegate(delegate);
		_recreateDisplayItem();
		_refreshDisplay();
		_updatePopupSize();
	}

	void ComboBox::setSelectedRow(std::optional<std::size_t> row)
	{
		_view.setSelectedRow(row);
	}

	void ComboBox::clearSelection()
	{
		setSelectedRow(std::nullopt);
	}

	void ComboBox::open()
	{
		_updatePopupSize();
		_popup.open();
		_updatePopupContentGeometry();
	}

	void ComboBox::close()
	{
		_popup.close();
	}

	void ComboBox::setPlaceholder(Font::Text placeholder)
	{
		_placeholder = std::move(placeholder);
		_refreshDisplay();
	}

	void ComboBox::setPlaceholder(std::string_view placeholder)
	{
		setPlaceholder(Font::textFromUTF8(placeholder));
	}

	void ComboBox::setMaximumVisibleRows(std::size_t count)
	{
		_maximumVisibleRows = count;
		_updatePopupSize();
	}

	void ComboBox::setOpenButtonWidth(unsigned int width)
	{
		if (_openButtonWidth == width)
		{
			return;
		}
		_openButtonWidth = width;
		_updateSizeHint();
		_onGeometryChange();
	}

	bool ComboBox::isOpen() const noexcept
	{
		return _popup.isOpen();
	}

	ComboBox::Model *ComboBox::model() const noexcept
	{
		return _view.model();
	}

	std::optional<std::size_t> ComboBox::selectedRow() const noexcept
	{
		return _view.selectedRow();
	}

	std::optional<ComboBox::Model::RowID> ComboBox::selectedRowID() const noexcept
	{
		return _view.selectedRowID();
	}

	Widget *ComboBox::selectedWidget() noexcept
	{
		return _view.selectedWidget();
	}

	const Widget *ComboBox::selectedWidget() const noexcept
	{
		return _view.selectedWidget();
	}

	std::optional<ComboBox::Selection> ComboBox::selection() const noexcept
	{
		return _view.selection();
	}

	const Font::Text &ComboBox::placeholder() const noexcept
	{
		return _placeholder;
	}

	std::size_t ComboBox::maximumVisibleRows() const noexcept
	{
		return _maximumVisibleRows;
	}

	unsigned int ComboBox::openButtonWidth() const noexcept
	{
		return _openButtonWidth;
	}

	Panel &ComboBox::background() noexcept
	{
		return _background;
	}

	const Panel &ComboBox::background() const noexcept
	{
		return _background;
	}

	TextLabel &ComboBox::displayLabel() noexcept
	{
		return _displayLabel;
	}

	const TextLabel &ComboBox::displayLabel() const noexcept
	{
		return _displayLabel;
	}

	Widget *ComboBox::displayItem() noexcept
	{
		return _displayItem.get();
	}

	const Widget *ComboBox::displayItem() const noexcept
	{
		return _displayItem.get();
	}

	PushButton &ComboBox::openButton() noexcept
	{
		return _openButton;
	}

	const PushButton &ComboBox::openButton() const noexcept
	{
		return _openButton;
	}

	ComboBox::ModelView &ComboBox::view() noexcept
	{
		return _view;
	}

	const ComboBox::ModelView &ComboBox::view() const noexcept
	{
		return _view;
	}

	Panel &ComboBox::popupBackground() noexcept
	{
		return _popupBackground;
	}

	const Panel &ComboBox::popupBackground() const noexcept
	{
		return _popupBackground;
	}

	PopupWidget &ComboBox::popup() noexcept
	{
		return _popup;
	}

	const PopupWidget &ComboBox::popup() const noexcept
	{
		return _popup;
	}

	ComboBox::SelectionContract ComboBox::subscribeToSelection(SelectionCallback callback)
	{
		return _selectionProvider.subscribe(std::move(callback));
	}
}
