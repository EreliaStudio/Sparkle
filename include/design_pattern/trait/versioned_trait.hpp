#pragma once

#include <cstdint>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	class VersionedTrait
	{
	public:
		using Version = std::uint64_t;
		using VersionProvider = ContractProvider<VersionedTrait *>;
		using Contract = VersionProvider::Contract;
		using callback_type = VersionProvider::callback_type;

	private:
		Version _version = 0;
		VersionProvider _versionProvider;

	public:
		VersionedTrait() = default;
		VersionedTrait(const VersionedTrait &) = delete;
		VersionedTrait(VersionedTrait &&other) noexcept;
		virtual ~VersionedTrait();

		VersionedTrait &operator=(const VersionedTrait &) = delete;
		VersionedTrait &operator=(VersionedTrait &&) = delete;

		void invalidate();
		[[nodiscard]] Version version() const noexcept;
		[[nodiscard]] Contract subscribeToVersionEdition(callback_type callback);

	};
}
