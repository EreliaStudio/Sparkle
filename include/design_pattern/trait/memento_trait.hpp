#pragma once

#include "design_pattern/memento/snapshot.hpp"

#include <functional>
#include <utility>

namespace spk
{
	// CRTP refers to the object, not to its (possibly nested) snapshot types.
	template <typename TObject>
	class MementoTrait
	{
	public:
		template <typename TState>
		[[nodiscard]] Memento::Snapshot<TState> save() const
		{
			return Memento::Snapshot<TState>(static_cast<const TObject &>(*this));
		}

		template <typename TState>
		void load(const Memento::Snapshot<TState> &snapshot)
		{
			snapshot.loadInto(static_cast<TObject &>(*this));
		}

		template <typename TState, typename TOperation>
		void transaction(TOperation &&operation)
		{
			const auto snapshot = save<TState>();
			try
			{
				std::invoke(std::forward<TOperation>(operation));
			}
			catch (...)
			{
				load(snapshot);
				throw;
			}
		}
	};
}
