#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace spk
{
	template <typename TObject>
	class MementoTrait;
}

namespace spk::Memento
{
	template <typename TState>
	class Snapshot final
	{
	private:
		TState _state;

		template <typename TObject>
			requires std::default_initializable<TState>
		explicit Snapshot(const TObject &object) :
			_state()
		{
			_state.saveFrom(object);
		}

		template <typename>
		friend class spk::MementoTrait;

	public:
		using State = TState;

		template <typename TObject>
		void loadInto(TObject &object) const
		{
			_state.loadInto(object);
		}
	};
}

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
