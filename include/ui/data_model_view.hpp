#pragma once

#include "container/data_model.hpp"
#include "ui/widget.hpp"

namespace spk
{
	template <typename TContent>
	class DataModel<TContent>::View :
		public Widget
	{
	public:
		class Delegate
		{
		public:
			virtual ~Delegate() = default;
			[[nodiscard]] virtual std::unique_ptr<Widget> createItem(std::string name, Widget *parent) = 0;
			virtual void bindItem(Widget &item, const DataModel &model, std::size_t row, bool selected) = 0;
			[[nodiscard]] virtual unsigned int rowExtent(const DataModel &model, std::size_t row) const = 0;
		};

		struct Selection
		{
			std::size_t row = 0;
			RowID rowID = 0;
			Widget *widget = nullptr;
			bool operator==(const Selection &) const = default;
		};
		using SelectionProvider = ContractProvider<std::optional<Selection>>;
		using SelectionCallback = typename SelectionProvider::callback_type;
		using SelectionContract = typename SelectionProvider::Contract;

	private:
		DataModel *_model = nullptr;
		Delegate *_delegate = nullptr;
		std::vector<std::unique_ptr<Widget>> _items;
		std::vector<std::optional<std::size_t>> _boundRows;
		std::optional<std::size_t> _selectedRow;
		std::optional<RowID> _selectedRowID;
		unsigned int _scrollOffset = 0;
		std::size_t _nextItemIdentifier = 0;
		RowsContract _insertedContract;
		RowsContract _removedContract;
		RowsContract _changedContract;
		ResetContract _resetContract;
		SelectionProvider _selectionProvider;

		void _detachModelContracts()
		{
			_insertedContract.resign();
			_removedContract.resign();
			_changedContract.resign();
			_resetContract.resign();
		}

		void _rebuildItems(bool notifySelection)
		{
			const std::optional<RowID> previousID = _selectedRowID;

			_items.clear();
			_boundRows.clear();
			_selectedRow.reset();
			_selectedRowID.reset();

			if (_model != nullptr && _delegate != nullptr && previousID.has_value())
			{
				if (const std::optional<std::size_t> row = _model->rowFromID(*previousID); row.has_value())
				{
					_selectedRow = row;
					_selectedRowID = *previousID;
				}
			}

			_updateSizeHint();
			_updateItemGeometry();

			if (notifySelection && previousID != _selectedRowID)
			{
				_selectionProvider.trigger(selection());
			}
		}

		[[nodiscard]] unsigned int _contentExtent() const
		{
			unsigned int result = 0;

			if (_model != nullptr && _delegate != nullptr)
			{
				for (std::size_t row = 0; row < _model->rowCount(); ++row)
				{
					result += _delegate->rowExtent(*_model, row);
				}
			}

			return result;
		}

		[[nodiscard]] std::optional<std::size_t> _rowAt(int globalY) const
		{
			if (_model == nullptr || _delegate == nullptr)
			{
				return std::nullopt;
			}

			const int local = globalY - viewRegion().viewport.y + static_cast<int>(_scrollOffset);

			if (local < 0)
			{
				return std::nullopt;
			}

			unsigned int cursor = 0;

			for (std::size_t row = 0; row < _model->rowCount(); ++row)
			{
				const unsigned int extent = _delegate->rowExtent(*_model, row);
				if (static_cast<unsigned int>(local) < cursor + extent)
				{
					return row;
				}
				cursor += extent;
			}

			return std::nullopt;
		}

		void _updateItemGeometry()
		{
			if (_model == nullptr || _delegate == nullptr)
			{
				return;
			}

			std::vector<std::pair<std::size_t, unsigned int>> desired;
			unsigned int top = 0;
			const unsigned int bottom = _scrollOffset + geometry().height;

			for (std::size_t row = 0; row < _model->rowCount(); ++row)
			{
				const unsigned int extent = _delegate->rowExtent(*_model, row);
				if (geometry().height != 0 && top + extent > _scrollOffset && top < bottom)
				{
					desired.emplace_back(row, top);
				}
				top += extent;
			}

			if (_selectedRow.has_value() && std::ranges::none_of(desired, [this](const auto &entry) {
					return entry.first == *_selectedRow;
				}))
			{
				unsigned int selectedTop = 0;
				for (std::size_t row = 0; row < *_selectedRow; ++row)
				{
					selectedTop += _delegate->rowExtent(*_model, row);
				}
				desired.emplace_back(*_selectedRow, selectedTop);
			}
			while (_items.size() < desired.size())
			{
				auto item = _delegate->createItem(name() + ".item-" + std::to_string(_nextItemIdentifier++), this);

				if (item == nullptr || item->parent() != this)
				{
					throw std::invalid_argument("DataModel view delegate item must be a non-null child of the view");
				}

				_items.push_back(std::move(item));
				_boundRows.push_back(std::nullopt);
			}
			for (std::size_t index = 0; index < desired.size(); ++index)
			{
				const auto match = std::find(_boundRows.begin() + static_cast<std::ptrdiff_t>(index), _boundRows.end(), std::optional<std::size_t>{desired[index].first});
				if (match != _boundRows.end())
				{
					const std::size_t source = static_cast<std::size_t>(std::distance(_boundRows.begin(), match));
					std::swap(_boundRows[index], _boundRows[source]);
					std::swap(_items[index], _items[source]);
				}
			}
			for (std::size_t index = 0; index < _items.size(); ++index)
			{
				if (index >= desired.size())
				{
					_boundRows[index].reset();
					_items[index]->deactivate();
					continue;
				}

				const auto [row, rowTop] = desired[index];
				_boundRows[index] = row;
				_items[index]->activate();
				_delegate->bindItem(*_items[index], *_model, row, _selectedRow == row);
				_items[index]->setGeometry(Rect2D{Vector2Int{0, static_cast<int>(rowTop) - static_cast<int>(_scrollOffset)}, Vector2UInt{geometry().width, _delegate->rowExtent(*_model, row)}});
			}
		}

		void _updateSizeHint() override
		{
			SizeHint hint = sizeHint();
			hint.minimal = {0, 0};
			hint.preferred = {0, static_cast<float>(_contentExtent())};
			hint.maximal = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			setSizeHint(hint);
		}

		void _onGeometryChange() override
		{
			_updateItemGeometry();
		}

		void _onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event) override
		{
			if (!viewRegion().viewport.contains(event.device.position) || event.record.value.y == 0)
			{
				return;
			}

			const unsigned int maximum = _contentExtent() > geometry().height ? _contentExtent() - geometry().height : 0;
			const int next = static_cast<int>(_scrollOffset) - static_cast<int>(std::lround(event.record.value.y * 20.0f));
			_scrollOffset = static_cast<unsigned int>(std::clamp(next, 0, static_cast<int>(maximum)));
			_updateItemGeometry();
			event.consumed = true;
		}

		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override
		{
			if (event.record.button != Mouse::Button::Left || !viewRegion().viewport.contains(event.device.position))
			{
				return;
			}

			const auto row = _rowAt(event.device.position.y);
			if (row.has_value() && _model->isEnabled(*row))
			{
				setSelectedRow(row);
			}
			event.consumed = true;
		}

	public:
		explicit View(std::string name, Widget *parent = nullptr) :
			Widget(std::move(name), parent)
		{
			activate();
		}

		void setModel(DataModel *model)
		{
			if (_model == model)
			{
				return;
			}

			_detachModelContracts();
			_model = model;
			if (_model != nullptr)
			{
				_insertedContract = _model->subscribeToRowsInserted([this](std::size_t, std::size_t) {
					_rebuildItems(true);
				});
				_removedContract = _model->subscribeToRowsRemoved([this](std::size_t, std::size_t) {
					_rebuildItems(true);
				});
				_changedContract = _model->subscribeToRowsChanged([this](std::size_t, std::size_t) {
					_rebuildItems(true);
				});
				_resetContract = _model->subscribeToReset([this]() {
					_rebuildItems(true);
				});
			}

			_selectedRow.reset();
			_selectedRowID.reset();
			_scrollOffset = 0;
			_rebuildItems(false);
		}

		void setDelegate(Delegate *delegate)
		{
			if (_delegate != delegate)
			{
				_delegate = delegate;
				_rebuildItems(false);
			}
		}

		void setSelectedRow(std::optional<std::size_t> row)
		{
			if (row.has_value() && (_model == nullptr || *row >= _model->rowCount()))
			{
				throw std::out_of_range("DataModel view selected row is out of range");
			}

			const auto id = row.has_value() ? std::optional<RowID>{_model->rowID(*row)} : std::nullopt;
			if (_selectedRow == row && _selectedRowID == id)
			{
				return;
			}

			_selectedRow = row;
			_selectedRowID = id;
			_updateItemGeometry();
			_selectionProvider.trigger(selection());
		}

		void scrollTo(std::size_t row)
		{
			if (_model == nullptr || _delegate == nullptr || row >= _model->rowCount())
			{
				throw std::out_of_range("DataModel view scroll row is out of range");
			}

			unsigned int top = 0;
			for (std::size_t index = 0; index < row; ++index)
			{
				top += _delegate->rowExtent(*_model, index);
			}

			const unsigned int extent = _delegate->rowExtent(*_model, row);
			if (top < _scrollOffset)
			{
				_scrollOffset = top;
			}
			else if (top + extent > _scrollOffset + geometry().height)
			{
				_scrollOffset = top + extent > geometry().height ? top + extent - geometry().height : 0;
			}

			_updateItemGeometry();
		}

		[[nodiscard]] DataModel *model() const noexcept
		{
			return _model;
		}

		[[nodiscard]] Delegate *delegate() const noexcept
		{
			return _delegate;
		}

		[[nodiscard]] std::optional<std::size_t> selectedRow() const noexcept
		{
			return _selectedRow;
		}

		[[nodiscard]] std::optional<RowID> selectedRowID() const noexcept
		{
			return _selectedRowID;
		}

		[[nodiscard]] Widget *selectedWidget() noexcept
		{
			if (!_selectedRow.has_value())
			{
				return nullptr;
			}

			for (std::size_t index = 0; index < _boundRows.size(); ++index)
			{
				if (_boundRows[index] == _selectedRow)
				{
					return _items[index].get();
				}
			}

			return nullptr;
		}

		[[nodiscard]] const Widget *selectedWidget() const noexcept
		{
			if (!_selectedRow.has_value())
			{
				return nullptr;
			}

			for (std::size_t index = 0; index < _boundRows.size(); ++index)
			{
				if (_boundRows[index] == _selectedRow)
				{
					return _items[index].get();
				}
			}

			return nullptr;
		}

		[[nodiscard]] std::optional<Selection> selection() const noexcept
		{
			return _selectedRow.has_value() ? std::optional<Selection>{{*_selectedRow, *_selectedRowID, const_cast<Widget *>(selectedWidget())}} : std::nullopt;
		}

		[[nodiscard]] SelectionContract subscribeToSelection(SelectionCallback callback)
		{
			return _selectionProvider.subscribe(std::move(callback));
		}
	};
}
