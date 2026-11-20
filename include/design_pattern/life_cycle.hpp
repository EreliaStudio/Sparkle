#pragma once

#include <functional>
#include <mutex>
#include <utility>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	template <typename TElement>
	class Pool;

	class LifeCycle
	{
	public:
		enum class State
		{
			Acquired,
			Released
		};

		using AcquisitionProvider = ContractProvider<>;
		using AcquisitionCallback = AcquisitionProvider::callback_type;
		using AcquisitionContract = AcquisitionProvider::Contract;
		using ReleaseProvider = ContractProvider<>;
		using ReleaseCallback = ReleaseProvider::callback_type;
		using ReleaseContract = ReleaseProvider::Contract;
		using DestructionProvider = ContractProvider<>;
		using DestructionCallback = DestructionProvider::callback_type;
		using DestructionContract = DestructionProvider::Contract;

	private:
		template <typename TElement>
		friend class Pool;

		mutable std::recursive_mutex _mutex;
		State _state = State::Acquired;
		bool _finalized = false;
		AcquisitionProvider _acquisitionProvider;
		ReleaseProvider _releaseProvider;
		DestructionProvider _destructionProvider;

	protected:
		void acquire()
		{
			const std::scoped_lock lock(_mutex);
			if (_finalized || _state == State::Acquired)
			{
				return;
			}
			_state = State::Acquired;
			_acquisitionProvider.trigger();
		}

		void release()
		{
			const std::scoped_lock lock(_mutex);
			if (_finalized || _state == State::Released)
			{
				return;
			}
			_state = State::Released;
			_releaseProvider.trigger();
		}

		// Derived owners call this before their members are destroyed.
		void finalizeLifeCycle() noexcept
		{
			const std::scoped_lock lock(_mutex);
			if (_finalized)
			{
				return;
			}
			_finalized = true;
			if (_state == State::Acquired)
			{
				_state = State::Released;
				try
				{
					_releaseProvider.trigger();
				} catch (...)
				{
					// Destruction must continue even when a subscriber fails.
				}
			}
			try
			{
				_destructionProvider.trigger();
			} catch (...)
			{
				// Destructors must not propagate subscriber exceptions.
			}
		}

	public:
		LifeCycle() = default;

		// Copies and moves are new objects; assignments preserve the destination's
		// current state and subscriptions rather than transferring observers.
		LifeCycle(const LifeCycle &) :
			LifeCycle()
		{
		}

		LifeCycle(LifeCycle &&) :
			LifeCycle()
		{
		}

		LifeCycle &operator=(const LifeCycle &) noexcept
		{
			return *this;
		}

		LifeCycle &operator=(LifeCycle &&) noexcept
		{
			return *this;
		}

		virtual ~LifeCycle() noexcept
		{
			finalizeLifeCycle();
		}

		[[nodiscard]] State state() const noexcept
		{
			const std::scoped_lock lock(_mutex);
			return _state;
		}

		[[nodiscard]] bool isAcquired() const noexcept
		{
			return state() == State::Acquired;
		}

		[[nodiscard]] bool isReleased() const noexcept
		{
			return state() == State::Released;
		}

		[[nodiscard]] AcquisitionContract subscribeToAcquisition(AcquisitionCallback callback)
		{
			return _acquisitionProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] ReleaseContract subscribeToRelease(ReleaseCallback callback)
		{
			return _releaseProvider.subscribe(std::move(callback));
		}

		[[nodiscard]] DestructionContract subscribeToDestruction(DestructionCallback callback)
		{
			return _destructionProvider.subscribe(std::move(callback));
		}
	};
}
