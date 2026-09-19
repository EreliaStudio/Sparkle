#pragma once

#include "concept.hpp"
#include "design_pattern/contract_provider.hpp"

#include <set>
#include <utility>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class QuerySourceTrait
	{
	public:
		using ElementSet = std::set<TType *>;
		using OnEditionContractProvider = ContractProvider<const TContext &>;
		using OnEditionCallback = typename OnEditionContractProvider::callback_type;
		using OnEditionContract = typename OnEditionContractProvider::Contract;

	private:
		OnEditionContractProvider _onEditionContractProvider;

	protected:
		void notifyEdition(const TContext &context)
		{
			_onEditionContractProvider.trigger(context);
		}

	public:
		virtual ~QuerySourceTrait() = default;

		[[nodiscard]] virtual const ElementSet &elements(const TContext &context) const = 0;

		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback)
		{
			return _onEditionContractProvider.subscribe(std::move(callback));
		}
	};
}
