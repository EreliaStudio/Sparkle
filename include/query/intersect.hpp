#pragma once

#include "query/where.hpp"

#include <concepts>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class Intersect : public Where<TType, TContext>
	{
	private:
		using Base = Where<TType, TContext>;
		using OnEditionContract = typename QuerySourceTrait<TType, TContext>::OnEditionContract;

		OnEditionContract _editionContract;

	public:
		template <typename TProvidedType>
			requires(std::same_as<TProvidedType, TType> || requires(TType *element) { dynamic_cast<TProvidedType *>(element); })
		explicit Intersect(QuerySourceTrait<TProvidedType, TContext> &source) :
			Base([&source](TType *element, const TContext &context) {
				const auto &providedElements = source.elements(context);
				if constexpr (std::same_as<TProvidedType, TType>)
				{
					return providedElements.contains(element);
				}
				else
				{
					TProvidedType *converted = dynamic_cast<TProvidedType *>(element);
					return converted != nullptr && providedElements.contains(converted);
				}
			}),
			_editionContract(source.subscribeToEdition(
				[this](const TContext &context) {
					this->invalidate(context);
				}))
		{
		}
	};
}
