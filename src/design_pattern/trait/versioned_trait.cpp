#include "design_pattern/trait/versioned_trait.hpp"

#include <utility>

namespace spk
{
	VersionedTrait::VersionedTrait(VersionedTrait &&other) noexcept :
		_version(std::exchange(other._version, 1))
	{
	}

	VersionedTrait::~VersionedTrait() = default;

	void VersionedTrait::invalidate()
	{
		++_version;
		if (_version == 0)
		{
			_version = 1;
		}
		_versionProvider.trigger(this);
	}

	VersionedTrait::Version VersionedTrait::version() const noexcept
	{
		return _version;
	}

	VersionedTrait::Contract VersionedTrait::subscribeToVersionEdition(callback_type callback)
	{
		return _versionProvider.subscribe(std::move(callback));
	}
}
