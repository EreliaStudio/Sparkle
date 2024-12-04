#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/design_pattern/spk_function_traits.hpp"
#include "structure/system/spk_exception.hpp" // GENERATE_ERROR

#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TType>
	class EventDispatcher
	{
	public:
		template <typename... TArgs>
		using Job = typename TContractProvider<TArgs...>::Job;

		template <typename... TArgs>
		using Contract = typename TContractProvider<TArgs...>::Contract;

	private:
		struct ProviderBase
		{
			virtual ~ProviderBase() = default;
		};

		template <typename... TArgs>
		struct Provider : ProviderBase
		{
			TContractProvider<TArgs...> contractProvider;
		};

		std::unordered_map<TType, std::unique_ptr<ProviderBase>> _providers;

		template <typename... TArgs>
		using DecayedProvider = Provider<std::decay_t<TArgs>...>;

		template <typename... TArgs>
		DecayedProvider<TArgs...> *_getTyped(const TType &event) noexcept
		{
			auto it = _providers.find(event);
			if (it == _providers.end())
			{
				return nullptr;
			}
			return dynamic_cast<DecayedProvider<TArgs...> *>(it->second.get());
		}

		template <typename... TArgs>
		DecayedProvider<TArgs...> *_ensureTyped(const TType &event)
		{
			auto &base = _providers[event];
			if (!base)
			{
				base = std::make_unique<DecayedProvider<TArgs...>>();
				return static_cast<DecayedProvider<TArgs...> *>(base.get());
			}

			auto *typed = dynamic_cast<DecayedProvider<TArgs...> *>(base.get());
			if (!typed)
			{
				GENERATE_ERROR("EventDispatcher: subscribe signature mismatch for event");
			}
			return typed;
		}

		template <typename... TArgs>
		void _emit(const TType &event, TArgs &&...args)
		{
			auto it = _providers.find(event);
			if (it == _providers.end())
			{
				return;
			}

			if (auto *typed = dynamic_cast<DecayedProvider<TArgs...> *>(it->second.get()))
			{
				typed->contractProvider.trigger(std::forward<TArgs>(args)...);
			}
			else
			{
				GENERATE_ERROR("EventDispatcher: emit signature mismatch for event");
			}
		}

		template <typename... TArgs>
		Contract<std::decay_t<TArgs>...> _subscribe(const TType &event, const Job<std::decay_t<TArgs>...> &job)
		{
			auto *typed = _ensureTyped<std::decay_t<TArgs>...>(event);
			return typed->contractProvider.subscribe(job);
		}

		template <typename... TArgs>
		void _invalidate(const TType &event)
		{
			if (auto *typed = _getTyped<std::decay_t<TArgs>...>(event))
			{
				typed->contractProvider.invalidateContracts();
			}
		}

		template <typename F, std::size_t... I>
		auto _subscribeFromCallableImpl(const TType &event, F &&f, std::index_sequence<I...>)
		{
			using Tuple = typename FunctionTraits<std::decay_t<F>>::ArgsTuple;
			using JobT = Job<std::decay_t<std::tuple_element_t<I, Tuple>>...>;

			JobT job(std::forward<F>(f));
			return _subscribe<std::decay_t<std::tuple_element_t<I, Tuple>>...>(event, job);
		}

		template <typename F>
		auto _subscribeFromCallable(const TType &event, F &&f)
		{
			using Tuple = typename FunctionTraits<std::decay_t<F>>::ArgsTuple;
			return _subscribeFromCallableImpl(event, std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}

	public:
		EventDispatcher() = default;

		EventDispatcher(const EventDispatcher &) = delete;
		EventDispatcher &operator=(const EventDispatcher &) = delete;

		EventDispatcher(EventDispatcher &&) noexcept = default;
		EventDispatcher &operator=(EventDispatcher &&) noexcept = default;

		// Instance API
		template <typename... TArgs>
		void emit(const TType &event, TArgs &&...args)
		{
			_emit<std::decay_t<TArgs>...>(event, std::forward<TArgs>(args)...);
		}

		template <typename... TArgs>
		Contract<std::decay_t<TArgs>...> subscribe(const TType &event, const Job<std::decay_t<TArgs>...> &job)
		{
			return _subscribe<std::decay_t<TArgs>...>(event, job);
		}

		template <typename F>
		auto subscribe(const TType &event, F &&f)
		{
			return _subscribeFromCallable(event, std::forward<F>(f));
		}

		template <typename... TArgs>
		void invalidateContracts(const TType &event)
		{
			_invalidate<std::decay_t<TArgs>...>(event);
		}
	};
}
