#include "network/replication/operation_guard.hpp"
#include "exception.hpp"

namespace spk::Network
{
	OperationGuard::OperationGuard(bool &active) :
		_active(active)
	{
		if (_active)
		{
			throw spk::Exception("Reentrant network operation");
		}
		_active = true;
	}

	OperationGuard::~OperationGuard()
	{
		_active = false;
	}
}
