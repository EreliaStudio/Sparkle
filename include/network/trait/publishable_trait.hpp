#pragma once

#include "design_pattern/trait/versioned_trait.hpp"
#include <memory>

namespace spk::Network
{
	// Owner-thread only. Registered objects retain a stable address.
	template <typename State>
	class PublishableTrait : public spk::VersionedTrait
	{
		std::shared_ptr<void> _lifetime = std::make_shared<int>(0);
		template <typename, typename>
		friend class PublicationSourceTrait;

	protected:
		[[nodiscard]] virtual State _buildNetworkState() const = 0;

	public:
		PublishableTrait() = default;
		PublishableTrait(const PublishableTrait &) = delete;
		PublishableTrait &operator=(const PublishableTrait &) = delete;
		~PublishableTrait() override = default;
		[[nodiscard]] State buildNetworkState() const
		{
			return _buildNetworkState();
		}
	};
}
