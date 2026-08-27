#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	template <typename TContent>
	class DataModel
	{
	public:
		using Content = TContent;
		class View;

		using RowID = std::uint64_t;
		using RowsProvider = ContractProvider<std::size_t, std::size_t>;
		using RowsCallback = typename RowsProvider::callback_type;
		using RowsContract = typename RowsProvider::Contract;
		using ResetProvider = ContractProvider<>;
		using ResetCallback = typename ResetProvider::callback_type;
		using ResetContract = typename ResetProvider::Contract;

	private:
		struct Row
		{
			RowID id;
			Content content;
			bool enabled = true;
		};

		std::vector<Row> _rows;
		RowID _nextRowID = 1;
		RowsProvider _rowsInsertedProvider;
		RowsProvider _rowsRemovedProvider;
		RowsProvider _rowsChangedProvider;
		ResetProvider _resetProvider;

		[[nodiscard]] RowID _takeNextRowID()
		{
			if (_nextRowID == 0)
			{
				throw std::overflow_error("DataModel exhausted its row identifiers");
			}
			return _nextRowID++;
		}

	public:
		DataModel() = default;

		explicit DataModel(std::vector<Content> contents)
		{
			_rows.reserve(contents.size());

			for (Content &content : contents)
			{
				_rows.push_back(Row{_takeNextRowID(), std::move(content), true});
			}
		}

		DataModel(std::initializer_list<Content> contents)
		{
			_rows.reserve(contents.size());

			for (const Content &content : contents)
			{
				_rows.push_back(Row{_takeNextRowID(), content, true});
			}
		}

		virtual ~DataModel() = default;

		[[nodiscard]] std::size_t rowCount() const noexcept
		{
			return _rows.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _rows.empty();
		}

		[[nodiscard]] RowID rowID(std::size_t row) const
		{
			return _rows.at(row).id;
		}

		[[nodiscard]] const Content &data(std::size_t row) const
		{
			return _rows.at(row).content;
		}

		[[nodiscard]] bool isEnabled(std::size_t row) const
		{
			return _rows.at(row).enabled;
		}

		[[nodiscard]] std::optional<std::size_t> rowFromID(RowID id) const noexcept
		{
			for (std::size_t row = 0; row < _rows.size(); ++row)
			{
				if (_rows[row].id == id)
				{
					return row;
				}
			}

			return std::nullopt;
		}

		template <typename... TArguments>
		RowID emplace(std::size_t row, TArguments &&...arguments)
		{
			if (row > _rows.size())
			{
				throw std::out_of_range("DataModel insertion row is out of range");
			}

			const RowID id = _takeNextRowID();
			_rows.emplace(_rows.begin() + static_cast<std::ptrdiff_t>(row), Row{id, Content(std::forward<TArguments>(arguments)...), true});
			_rowsInsertedProvider.trigger(row, 1);
			return id;
		}

		template <typename... TArguments>
		RowID emplaceBack(TArguments &&...arguments)
		{
			return emplace(_rows.size(), std::forward<TArguments>(arguments)...);
		}

		RowID insert(std::size_t row, Content content)
		{
			return emplace(row, std::move(content));
		}

		RowID append(Content content)
		{
			return emplaceBack(std::move(content));
		}

		void erase(std::size_t first, std::size_t count = 1)
		{
			if (first > _rows.size() || count > _rows.size() - first)
			{
				throw std::out_of_range("DataModel erased row range is out of range");
			}

			if (count == 0)
			{
				return;
			}

			const auto begin = _rows.begin() + static_cast<std::ptrdiff_t>(first);
			_rows.erase(begin, begin + static_cast<std::ptrdiff_t>(count));
			_rowsRemovedProvider.trigger(first, count);
		}

		void setData(std::size_t row, Content content)
		{
			_rows.at(row).content = std::move(content);
			_rowsChangedProvider.trigger(row, 1);
		}

		void setEnabled(std::size_t row, bool enabled)
		{
			Row &entry = _rows.at(row);
			if (entry.enabled == enabled)
			{
				return;
			}

			entry.enabled = enabled;
			_rowsChangedProvider.trigger(row, 1);
		}

		void clear()
		{
			if (_rows.empty())
			{
				return;
			}

			_rows.clear();
			_resetProvider.trigger();
		}

		[[nodiscard]] RowsContract subscribeToRowsInserted(RowsCallback callback)
		{
			return _rowsInsertedProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] RowsContract subscribeToRowsRemoved(RowsCallback callback)
		{
			return _rowsRemovedProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] RowsContract subscribeToRowsChanged(RowsCallback callback)
		{
			return _rowsChangedProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] ResetContract subscribeToReset(ResetCallback callback)
		{
			return _resetProvider.subscribe(std::move(callback));
		}
	};
}
