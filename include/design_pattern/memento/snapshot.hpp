#pragma once

#include <concepts>

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
