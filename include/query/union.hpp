#pragma once

#include "container/query.hpp"

#include <concepts>
#include <functional>

namespace spk
{
	template <typename TType, typename TContext>
		requires Hashable<TContext>
	class Union : public Query<TType, TContext>::Operation
	{
	private:
		using QueryType = Query<TType, TContext>;
		using ElementSet = typename QueryType::ElementSet;
		using OnEditionContract = typename QuerySourceTrait<TType, TContext>::OnEditionContract;

		std::function<void(ElementSet &, const TContext &)> _append;
		OnEditionContract _editionContract;

	public:
		template <typename TProvidedType>
			requires(std::same_as<TProvidedType, TType> || requires(TProvidedType *element) { dynamic_cast<TType *>(element); })
		explicit Union(QuerySourceTrait<TProvidedType, TContext> &source)
		{
			_append = [&source](ElementSet &currentElements, const TContext &context) {
				const auto &providedElements = source.elements(context);

				if constexpr (std::same_as<TProvidedType, TType>)
				{
					currentElements.insert(providedElements.begin(), providedElements.end());
				}
				else
				{
					for (TProvidedType *element : providedElements)
					{
						if (TType *converted = dynamic_cast<TType *>(element); converted != nullptr)
						{
							currentElements.insert(converted);
						}
					}
				}
			};

			_editionContract = source.subscribeToEdition(
				[this](const TContext &context) {
					this->invalidate(context);
				});
		}

		void execute(ElementSet &currentElements, const TContext &context) override
		{
			_append(currentElements, context);
		}
	};
}
