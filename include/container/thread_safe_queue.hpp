#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <utility>

namespace spk
{
	template <typename TValue>
	class ThreadSafeQueue final
	{
	public:
		using value_type = TValue;
		using container_type = std::queue<value_type>;

	private:
		struct State
		{
			mutable std::mutex mutex;
			std::condition_variable_any condition;
			container_type values;

			void publish(value_type value)
			{
				{
					const std::scoped_lock lock(mutex);
					values.push(std::move(value));
				}

				condition.notify_one();
			}

			template <typename... TArguments>
			void emplace(TArguments &&...arguments)
			{
				{
					const std::scoped_lock lock(mutex);
					values.emplace(
						std::forward<TArguments>(arguments)...);
				}

				condition.notify_one();
			}

			[[nodiscard]] std::optional<value_type> waitPop(
				std::stop_token stopToken = {})
			{
				std::unique_lock lock(mutex);
				if (!condition.wait(
						lock,
						stopToken,
						[this] {
							return !values.empty();
						}))
				{
					return std::nullopt;
				}

				value_type value = std::move(values.front());
				values.pop();
				return value;
			}
		};

	public:
		class Producer
		{
		private:
			std::shared_ptr<State> _state;

			explicit Producer(std::shared_ptr<State> state) :
				_state(std::move(state))
			{
			}

			friend class ThreadSafeQueue;

		public:
			void publish(value_type value)
			{
				_state->publish(std::move(value));
			}

			template <typename... TArguments>
			void emplace(TArguments &&...arguments)
			{
				_state->emplace(
					std::forward<TArguments>(arguments)...);
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

			friend class ThreadSafeQueue;

		public:
			Consumer(const Consumer &) = delete;
			Consumer(Consumer &&) = default;

			Consumer &operator=(const Consumer &) = delete;
			Consumer &operator=(Consumer &&) = default;

			[[nodiscard]] std::optional<value_type> waitPop(
				std::stop_token stopToken = {})
			{
				return _state->waitPop(stopToken);
			}
		};

		struct Endpoints
		{
			Producer producer;
			Consumer consumer;
		};

	private:
		std::shared_ptr<State> _state;

	public:
		ThreadSafeQueue() :
			_state(std::make_shared<State>())
		{
		}

		void publish(value_type value)
		{
			_state->publish(std::move(value));
		}

		template <typename... TArguments>
		void emplace(TArguments &&...arguments)
		{
			_state->emplace(std::forward<TArguments>(arguments)...);
		}

		[[nodiscard]] std::optional<value_type> waitPop(
			std::stop_token stopToken = {})
		{
			return _state->waitPop(stopToken);
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
