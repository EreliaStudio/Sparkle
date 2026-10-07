#include "design_pattern/trait/versioned_trait.hpp"
#include "exception.hpp"

#include <limits>
#include <utility>

namespace spk
{
	VersionedTrait::VersionedTrait(Version initialVersion) :
		_version(initialVersion)
	{
	}

	VersionedTrait::VersionedTrait(VersionedTrait &&other) noexcept :
		_version(std::exchange(other._version, 1))
	{
	}

	VersionedTrait::~VersionedTrait() = default;

	void VersionedTrait::invalidate()
	{
		if (_version == std::numeric_limits<Version>::max())
		{
			throw spk::Exception("Version exhausted");
		}
		++_version;
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
