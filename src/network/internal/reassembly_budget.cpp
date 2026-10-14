#include "network/internal/reassembly_budget.hpp"

#include "exception.hpp"

#include <algorithm>
#include <cassert>
#include <mutex>

namespace spk::NetworkInternal
{
	struct ReassemblyBudget::Data
	{
		mutable std::mutex mutex;
		std::size_t maximum = 0;
		std::size_t used = 0;
		std::list<Lease *> leases;
	};

	ReassemblyBudget::Lease::~Lease()
	{
		if (_data == nullptr)
		{
			return;
		}
		const std::scoped_lock lock(_data->mutex);
		_data->used -= _bytes;
		_data->leases.erase(_registration);
	}

	bool ReassemblyBudget::Lease::reserve(std::size_t bytes)
	{
		const std::scoped_lock lock(_data->mutex);
		if (_data->used > _maximum || bytes > _maximum - _data->used)
		{
			return false;
		}
		_data->used += bytes;
		_bytes += bytes;
		return true;
	}

	void ReassemblyBudget::Lease::release(std::size_t bytes) noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		assert(bytes <= _bytes);
		_data->used -= bytes;
		_bytes -= bytes;
	}

	ReassemblyBudget::ReassemblyBudget(std::size_t maximum)
	{
		if (maximum == 0)
		{
			throw Exception("Reassembly memory budget must be positive.");
		}
		_data = std::make_shared<Data>();
		_data->maximum = maximum;
	}

	std::unique_ptr<ReassemblyBudget::Lease> ReassemblyBudget::reserve(std::size_t bytes)
	{
		auto lease = std::unique_ptr<Lease>(new Lease());
		const std::scoped_lock lock(_data->mutex);
		if (_data->used > _data->maximum || bytes > _data->maximum - _data->used)
		{
			return nullptr;
		}
		lease->_registration = _data->leases.insert(_data->leases.end(), lease.get());
		lease->_data = _data;
		lease->_maximum = _data->maximum;
		lease->_bytes = bytes;
		_data->used += bytes;
		return lease;
	}

	void ReassemblyBudget::setMaximum(std::size_t bytes)
	{
		if (bytes == 0)
		{
			throw Exception("Reassembly memory budget must be positive.");
		}
		const std::scoped_lock lock(_data->mutex);
		_data->maximum = bytes;
		for (Lease *lease : _data->leases)
		{
			lease->_maximum = std::max(lease->_maximum, bytes);
		}
	}

	std::size_t ReassemblyBudget::used() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->used;
	}
}
