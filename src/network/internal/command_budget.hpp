#pragma once

#include "exception.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>

namespace spk::NetworkInternal
{
	class CommandBudget final
	{
	public:
		enum class Stage
		{
			Command,
			Transfer,
			Frame,
			InFlight
		};

	private:
		struct Data final
		{
			mutable std::mutex mutex;
			std::size_t maximum;
			std::size_t used = 0;
			std::array<std::size_t, 4> byStage{};
		};

	public:
		class Lease final
		{
			friend class CommandBudget;

		private:
			std::shared_ptr<Data> _data;
			std::size_t _bytes = 0;
			Stage _stage = Stage::Command;

		public:
			Lease() = default;
			Lease(const Lease &) = delete;
			Lease &operator=(const Lease &) = delete;

			~Lease()
			{
				if (_data != nullptr)
				{
					const std::scoped_lock lock(_data->mutex);
					_data->used -= _bytes;
					_data->byStage[static_cast<std::size_t>(_stage)] -= _bytes;
				}
			}

			[[nodiscard]] std::size_t bytes() const noexcept
			{
				return _bytes;
			}

			void transition(Stage stage) noexcept
			{
				if (_data == nullptr)
				{
					return;
				}
				const std::scoped_lock lock(_data->mutex);
				_data->byStage[static_cast<std::size_t>(_stage)] -= _bytes;
				_data->byStage[static_cast<std::size_t>(stage)] += _bytes;
				_stage = stage;
			}
		};

	private:
		std::shared_ptr<Data> _data;

	public:
		explicit CommandBudget(std::size_t maximum) :
			_data(std::make_shared<Data>())
		{
			_data->maximum = maximum;
		}

		[[nodiscard]] std::shared_ptr<Lease> reserve(std::size_t bytes, Stage stage = Stage::Command)
		{
			auto lease = std::make_shared<Lease>();
			const std::scoped_lock lock(_data->mutex);
			if (_data->used > _data->maximum || bytes > _data->maximum - _data->used)
			{
				throw Exception("Network outgoing memory budget is saturated.");
			}
			lease->_data = _data;
			lease->_bytes = bytes;
			lease->_stage = stage;
			_data->used += bytes;
			_data->byStage[static_cast<std::size_t>(stage)] += bytes;
			return lease;
		}

		void setMaximum(std::size_t maximum) noexcept
		{
			const std::scoped_lock lock(_data->mutex);
			_data->maximum = maximum;
		}

		[[nodiscard]] std::size_t used() const noexcept
		{
			const std::scoped_lock lock(_data->mutex);
			return _data->used;
		}

		[[nodiscard]] std::size_t used(Stage stage) const noexcept
		{
			const std::scoped_lock lock(_data->mutex);
			return _data->byStage[static_cast<std::size_t>(stage)];
		}
	};
}
