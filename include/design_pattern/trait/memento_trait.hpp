#pragma once
#include <memory>

namespace spk
{
	// Explicit, application-owned snapshots. No automatic transaction or network coupling.
	class MementoTrait
	{
	public:
		class Snapshot
		{
		public:
			virtual ~Snapshot();
		};

	protected:
		// Snapshots own the data required for restoration; they must not borrow mutable object state.
		[[nodiscard]] virtual std::unique_ptr<const Snapshot> _saveSnapshot() const = 0;
		// The implementation validates the snapshot type and restores its own invariants.
		virtual void _restoreSnapshot(const Snapshot &snapshot) = 0;

	public:
		virtual ~MementoTrait() = default;
		[[nodiscard]] std::unique_ptr<const Snapshot> saveSnapshot() const;
		void restoreSnapshot(const Snapshot &snapshot);
	};
}
