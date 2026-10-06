#pragma once

#include "network/replication/sequence.hpp"
#include <memory>

namespace spk::Network
{
	// Owner-thread only. Registered objects retain a stable address.
	template <typename State>
	class PublishableTrait
	{
		Sequence _edition;
		std::shared_ptr<void> _lifetime = std::make_shared<int>(0);
		template <typename, typename>
		friend class PublicationSourceTrait;

	protected:
		[[nodiscard]] virtual State _buildNetworkState() const = 0;

	public:
		PublishableTrait() = default;
		PublishableTrait(const PublishableTrait &) = delete;
		PublishableTrait &operator=(const PublishableTrait &) = delete;
		virtual ~PublishableTrait() = default;
		void invalidateNetworkState()
		{
			(void)_edition.next();
		}
		[[nodiscard]] std::uint64_t networkEdition() const noexcept
		{
			return _edition.value();
		}
		[[nodiscard]] State buildNetworkState() const
		{
			return _buildNetworkState();
		}
	};
}
