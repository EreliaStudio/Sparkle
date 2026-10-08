#pragma once

namespace spk::Network
{
	// Reject owner-thread reentry while an operation calls application hooks.
	class OperationGuard final
	{
		bool &_active;

	public:
		explicit OperationGuard(bool &active);
		~OperationGuard();
		OperationGuard(const OperationGuard &) = delete;
		OperationGuard &operator=(const OperationGuard &) = delete;
	};
}
