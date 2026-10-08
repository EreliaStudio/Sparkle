#pragma once
#include "design_pattern/trait/versioned_trait.hpp"
#include "network/message.hpp"
#include <memory>
namespace spk::Network
{
	// Owner-thread only. Registered objects retain a stable address.
	class PublishableTrait : public spk::VersionedTrait
	{
		std::shared_ptr<void> _lifetime = std::make_shared<int>(0);
		friend class PublishedObjectObservationTrait;

	protected:
		virtual void _writeNetworkState(spk::Message::Writer &writer) const = 0;

	public:
		PublishableTrait() = default;
		PublishableTrait(const PublishableTrait &) = delete;
		PublishableTrait &operator=(const PublishableTrait &) = delete;
		~PublishableTrait() override = default;
		virtual void writeNetworkState(spk::Message::Writer &writer) const final;
		[[nodiscard]] spk::Message captureNetworkState() const;
	};
}
