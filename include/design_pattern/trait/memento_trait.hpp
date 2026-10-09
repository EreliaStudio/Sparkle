#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace spk
{
	template <typename TObject>
	class MementoTrait
	{
	public:
		template <typename TState>
		class Snapshot final
		{
		private:
			TState _state;

			explicit Snapshot(TState state) :
				_state(std::move(state))
			{
			}

			friend class MementoTrait<TObject>;

		public:
			using State = TState;

			[[nodiscard]] const State &state() const noexcept
			{
				return _state;
			}
		};

		template <typename TState>
			requires std::default_initializable<TState>
		[[nodiscard]] Snapshot<TState> save() const
		{
			TState state{};
			state.capture(static_cast<const TObject &>(*this));
			return Snapshot<TState>(std::move(state));
		}

		template <typename TState>
		void load(const Snapshot<TState> &snapshot)
		{
			snapshot._state.restore(static_cast<TObject &>(*this));
		}

		template <typename TState, typename TOperation>
		void transaction(TOperation &&operation)
		{
			const auto snapshot = save<TState>();
			try
			{
				std::invoke(std::forward<TOperation>(operation));
			} catch (...)
			{
				load(snapshot);
				throw;
			}
		}
	};
}
