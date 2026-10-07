#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"

namespace spk
{
	MementoTrait::Snapshot::~Snapshot() = default;
	std::unique_ptr<const MementoTrait::Snapshot> MementoTrait::saveSnapshot() const
	{
		auto snapshot = _saveSnapshot();
		if (!snapshot)
		{
			throw spk::Exception("Cannot save an empty memento");
		}
		return snapshot;
	}
	void MementoTrait::restoreSnapshot(const Snapshot &snapshot)
	{
		_restoreSnapshot(snapshot);
	}
}
