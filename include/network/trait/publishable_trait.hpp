#pragma once

#include "design_pattern/contract_provider.hpp"
#include <memory>

namespace spk::Network
{
	template <typename State, typename Codec>
	class PublicationSourceTrait;

	// Owner-thread only. Registered objects have stable addresses and produce
	// snapshots without mutable aliases into the live object.
	template <typename State>
	class PublishableTrait
	{
	public:
		using EditionProvider = spk::ContractProvider<>;
		using EditionContract = typename EditionProvider::Contract;

	private:
		std::shared_ptr<void> _lifetime = std::make_shared<int>(0);
		EditionProvider _editions;
		template <typename, typename>
		friend class PublicationSourceTrait;

	protected:
		[[nodiscard]] virtual State _buildNetworkState() const = 0;

	public:
		PublishableTrait() = default;
		PublishableTrait(const PublishableTrait &) = delete;
		PublishableTrait &operator=(const PublishableTrait &) = delete;
		PublishableTrait(PublishableTrait &&) = delete;
		PublishableTrait &operator=(PublishableTrait &&) = delete;
		virtual ~PublishableTrait() = default;

		void invalidateNetworkState()
		{
			_editions.trigger();
		}

		[[nodiscard]] virtual State buildNetworkState() const final
		{
			return _buildNetworkState();
		}

		[[nodiscard]] EditionContract subscribeToNetworkEdition(typename EditionProvider::callback_type callback)
		{
			return _editions.subscribe(std::move(callback));
		}
	};
}
