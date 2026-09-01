#pragma once

#include "concept.hpp"
#include "design_pattern/contract_provider.hpp"

#include <utility>

namespace spk
{
	template <typename TContext>
		requires Hashable<TContext>
	class ContextualizableTrait
	{
	public:
		using Context = TContext;
		using OnContextEditionContractProvider = ContractProvider<const TContext &, const TContext &>;
		using OnContextEditionContract = typename OnContextEditionContractProvider::Contract;
		using OnContextEditionCallback = typename OnContextEditionContractProvider::callback_type;

	private:
		TContext _context;
		OnContextEditionContractProvider _onContextEditionContractProvider;

	public:
		explicit ContextualizableTrait(const TContext &context = {}) :
			_context(context)
		{
		}

		[[nodiscard]] OnContextEditionContract subscribeToContextEdition(OnContextEditionCallback callback)
		{
			return _onContextEditionContractProvider.subscribe(std::move(callback));
		}

		void changeContext(const TContext &newContext)
		{
			if (_context == newContext)
			{
				return;
			}

			_onContextEditionContractProvider.trigger(_context, newContext);
			_context = newContext;
		}

		[[nodiscard]] const TContext &context() const
		{
			return _context;
		}
	};
}
