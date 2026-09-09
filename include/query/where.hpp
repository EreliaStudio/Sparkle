#pragma once

#include "container/query.hpp"

#include <concepts>
#include <functional>
#include <utility>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class Where : public Query<TType, TContext>::Operation
	{
	private:
		using ElementSet = typename Query<TType, TContext>::ElementSet;
		std::function<bool(TType *, const TContext &)> _predicate;

	public:
		template <typename TPredicate>
			requires(std::predicate<const TPredicate &, TType *> || std::predicate<const TPredicate &, TType *, const TContext &>)
		explicit Where(TPredicate predicate)
		{
			if constexpr (std::predicate<const TPredicate &, TType *, const TContext &>)
			{
				_predicate = std::move(predicate);
			}
			else
			{
				_predicate = [predicate = std::move(predicate)](TType *element, const TContext &) {
					return predicate(element);
				};
			}
		}

		void execute(ElementSet &currentElements, const TContext &context) override
		{
			for (auto it = currentElements.begin(); it != currentElements.end();)
			{
				it = _predicate(*it, context) ? std::next(it) : currentElements.erase(it);
			}
		}
	};
}
