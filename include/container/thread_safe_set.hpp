#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <unordered_set>
#include <utility>

namespace spk
{
	template <
		typename TValue,
		typename THash = std::hash<TValue>,
		typename TEqual = std::equal_to<TValue>>
	class ThreadSafeSet final
	{
	public:
		using value_type = TValue;
		using container_type = std::unordered_set<value_type, THash, TEqual>;

	private:
		struct State
		{
			mutable std::mutex mutex;
			std::condition_variable_any condition;
			container_type values;

			[[nodiscard]] bool publish(value_type value)
			{
				bool inserted = false;
				{
					const std::scoped_lock lock(mutex);
					inserted = values.insert(std::move(value)).second;
				}

				if (inserted)
				{
					condition.notify_one();
				}
				return inserted;
			}

			template <typename... TArguments>
			[[nodiscard]] bool emplace(TArguments &&...arguments)
			{
				bool inserted = false;
				{
					const std::scoped_lock lock(mutex);
					inserted = values.emplace(std::forward<TArguments>(arguments)...).second;
				}

				if (inserted)
				{
					condition.notify_one();
				}
				return inserted;
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

			[[nodiscard]] bool contains(const value_type &value) const
			{
				const std::scoped_lock lock(mutex);
				return values.contains(value);
			}

			[[nodiscard]] bool erase(const value_type &value)
			{
				const std::scoped_lock lock(mutex);
				return values.erase(value) != 0u;
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

			friend class ThreadSafeSet;

		public:
			[[nodiscard]] bool publish(value_type value)
			{
				return _state->publish(std::move(value));
			}

			template <typename... TArguments>
			[[nodiscard]] bool emplace(TArguments &&...arguments)
			{
				return _state->emplace(std::forward<TArguments>(arguments)...);
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

			friend class ThreadSafeSet;

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

	private:
		std::shared_ptr<State> _state;

	public:
		ThreadSafeSet() :
			_state(std::make_shared<State>())
		{
		}

		[[nodiscard]] bool publish(value_type value)
		{
			return _state->publish(std::move(value));
		}

		template <typename... TArguments>
		[[nodiscard]] bool emplace(TArguments &&...arguments)
		{
			return _state->emplace(std::forward<TArguments>(arguments)...);
		}

		[[nodiscard]] bool wait(std::stop_token stopToken = {})
		{
			return _state->wait(stopToken);
		}

		[[nodiscard]] container_type &drain(container_type &toFill)
		{
			return _state->drain(toFill);
		}

		[[nodiscard]] bool contains(const value_type &value) const
		{
			return _state->contains(value);
		}

		[[nodiscard]] bool erase(const value_type &value)
		{
			return _state->erase(value);
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
