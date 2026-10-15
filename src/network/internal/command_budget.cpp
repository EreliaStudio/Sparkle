#include "network/internal/command_budget.hpp"

#include "exception.hpp"

namespace spk::NetworkInternal
{
	CommandBudget::Lease::~Lease()
	{
		if (_data != nullptr)
		{
			const std::scoped_lock lock(_data->mutex);
			_data->used -= _bytes;
			_data->byStage[static_cast<std::size_t>(_stage)] -= _bytes;
		}
	}

	std::size_t CommandBudget::Lease::bytes() const noexcept
	{
		return _bytes;
	}

	void CommandBudget::Lease::transition(Stage stage) noexcept
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

	CommandBudget::CommandBudget(std::size_t maximum) :
		_data(std::make_shared<Data>())
	{
		_data->maximum = maximum;
	}

	std::shared_ptr<CommandBudget::Lease> CommandBudget::reserve(std::size_t bytes, Stage stage)
	{
		auto lease = tryReserve(bytes, stage);
		if (lease == nullptr)
		{
			throw Exception("Network outgoing memory budget is saturated.");
		}
		return lease;
	}

	std::shared_ptr<CommandBudget::Lease> CommandBudget::tryReserve(std::size_t bytes, Stage stage)
	{
		auto lease = std::make_shared<Lease>();
		const std::scoped_lock lock(_data->mutex);
		if (_data->used > _data->maximum || bytes > _data->maximum - _data->used)
		{
			return nullptr;
		}
		lease->_data = _data;
		lease->_bytes = bytes;
		lease->_stage = stage;
		_data->used += bytes;
		_data->byStage[static_cast<std::size_t>(stage)] += bytes;
		return lease;
	}

	void CommandBudget::setMaximum(std::size_t maximum) noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		_data->maximum = maximum;
	}

	std::size_t CommandBudget::used() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->used;
	}

	std::size_t CommandBudget::used(Stage stage) const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->byStage[static_cast<std::size_t>(stage)];
	}
}
