#pragma once

#include <string>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	class NameTrait
	{
	public:
		using NameEditionProvider = ContractProvider<const std::string &>;
		using NameEditionCallback = NameEditionProvider::callback_type;
		using NameEditionContract = NameEditionProvider::Contract;

	private:
		std::string _name;
		NameEditionProvider _nameEditionProvider;

	public:
		NameTrait(std::string name);
		NameTrait(const NameTrait &other);
		NameTrait(NameTrait &&other) noexcept;
		NameTrait &operator=(const NameTrait &other);
		NameTrait &operator=(NameTrait &&other);

		void setName(std::string name);
		[[nodiscard]] const std::string &name() const noexcept;
		[[nodiscard]] NameEditionContract subscribeToNameEdition(NameEditionCallback callback);
	};
}
