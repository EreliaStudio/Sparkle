#pragma once

#include <concepts>
#include <utility>

namespace spk::Memento
{
	template <typename TState>
	class Snapshot final
	{
	private:
		TState _state;

		template <typename TObject>
		explicit Snapshot(const TObject &object) requires std::default_initializable<TState> :
			_state()
		{
			_state.saveFrom(object);
		}

		template <typename>
		friend class spk::Memento::Snapshot;

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
