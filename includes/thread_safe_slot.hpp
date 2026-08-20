#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace spk
{
	template <typename TValue>
	class ThreadSafeSlot final
	{
	public:
		using value_type = TValue;
		using pointer = std::shared_ptr<const value_type>;

	private:
		struct State
		{
			std::atomic<pointer> latest = nullptr;

			void publish(value_type value)
			{
				latest.store(std::make_shared<value_type>(std::move(value)), std::memory_order_release);
			}

			[[nodiscard]] pointer acquireLatest() const noexcept
			{
				return latest.load(std::memory_order_acquire);
			}
		};

		std::shared_ptr<State> _state;

	public:
		class Producer
		{
		private:
			std::shared_ptr<State> _state;

			explicit Producer(std::shared_ptr<State> state) :
				_state(std::move(state))
			{
			}

			friend class ThreadSafeSlot;

		public:
			void publish(value_type value)
			{
				_state->publish(std::move(value));
			}
		};

		class Consumer
		{
		private:
			std::shared_ptr<State> _state;

			explicit Consumer(std::shared_ptr<State> state) :
				_state(std::move(state))
			{
			}

			friend class ThreadSafeSlot;

		public:
			[[nodiscard]] pointer acquireLatest() const noexcept
			{
				return _state->acquireLatest();
			}
		};

		struct Endpoints
		{
			Producer producer;
			Consumer consumer;
		};

		ThreadSafeSlot() :
			_state(std::make_shared<State>())
		{
		}

		void publish(value_type value)
		{
			_state->publish(std::move(value));
		}

		[[nodiscard]] pointer acquireLatest() const noexcept
		{
			return _state->acquireLatest();
		}

		[[nodiscard]] static Endpoints create()
		{
			auto state = std::make_shared<State>();

			return {
				.producer = Producer(state),
				.consumer = Consumer(std::move(state))};
		}

		[[nodiscard]] Producer producer() const
		{
			return Producer(_state);
		}

		[[nodiscard]] Consumer consumer() const
		{
			return Consumer(_state);
		}
	};
}