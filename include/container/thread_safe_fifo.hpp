#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TValue>
	class ThreadSafeFIFO final
	{
	public:
		using value_type = TValue;
		using container_type = std::vector<value_type>;

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
					values.push_back(std::move(value));
				}

				condition.notify_one();
			}

			template <typename... TArguments>
			void emplace(TArguments &&...arguments)
			{
				{
					const std::scoped_lock lock(mutex);

					values.emplace_back(
						std::forward<TArguments>(arguments)...);
				}

				condition.notify_one();
			}

			[[nodiscard]] bool wait(std::stop_token stopToken = {})
			{
				std::unique_lock lock(mutex);

				return condition.wait(
					lock,
					stopToken,
					[this] {
						return !values.empty();
					});
			}

			[[nodiscard]] container_type &drain(container_type &toFill)
			{
				const std::scoped_lock lock(mutex);

				toFill.clear();
				toFill.swap(values);

				return toFill;
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

			friend class ThreadSafeFIFO;

		public:
			void publish(value_type value)
			{
				_state->publish(std::move(value));
			}

			template <typename... TArguments>
			void emplace(TArguments &&...arguments)
			{
				_state->emplace(std::forward<TArguments>(arguments)...);
			}
		};

		class Consumer
		{
		private:
			std::shared_ptr<State> _state;
			container_type _values;

			explicit Consumer(std::shared_ptr<State> state) :
				_state(std::move(state))
			{
			}

			friend class ThreadSafeFIFO;

		public:
			Consumer(const Consumer &) = delete;
			Consumer(Consumer &&) = default;

			Consumer &operator=(const Consumer &) = delete;
			Consumer &operator=(Consumer &&) = default;

			[[nodiscard]] bool wait(std::stop_token stopToken = {})
			{
				return _state->wait(stopToken);
			}

			[[nodiscard]] container_type &drain()
			{
				return _state->drain(_values);
			}
		};

		struct Endpoints
		{
			Producer producer;
			Consumer consumer;
		};

		ThreadSafeFIFO() :
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

		[[nodiscard]] bool wait(std::stop_token stopToken = {})
		{
			return _state->wait(stopToken);
		}

		[[nodiscard]] container_type &drain(container_type &toFill)
		{
			return _state->drain(toFill);
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