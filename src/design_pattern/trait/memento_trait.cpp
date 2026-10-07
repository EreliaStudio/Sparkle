#include "design_pattern/trait/memento_trait.hpp"

namespace spk
{
	MementoTrait::Snapshot MementoTrait::save() const
	{
		return _save();
	}
	void MementoTrait::restore(const Snapshot &snapshot)
	{
		_restore(snapshot);
	}
}
