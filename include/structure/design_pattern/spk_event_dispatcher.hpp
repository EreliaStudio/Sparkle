#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/design_pattern/spk_function_traits.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TType>
	class EventDispatcher : public spk::Singleton<EventDispatcher<TType>>
	{
		friend class spk::Singleton<EventDispatcher<TType>>;

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
		struct Provider : public ProviderBase
		{
			TContractProvider<TArgs...> contractProvider;
		};

		std::unordered_map<TType, std::unique_ptr<ProviderBase>> _providers;

		EventDispatcher() = default;

		template <typename... TArgs>
		void _emit(const TType &p_event, TArgs &&...p_args)
		{
			using DecayedProvider = Provider<std::decay_t<TArgs>...>;

			auto it = _providers.find(p_event);
			if (it != _providers.end())
			{
				if (auto *typed = dynamic_cast<DecayedProvider *>(it->second.get()))
				{
					typed->contractProvider.trigger(std::forward<TArgs>(p_args)...);
				}
			}
		}

		template <typename... TArgs>
		Contract<std::decay_t<TArgs>...> _subscribe(const TType &p_event, const Job<std::decay_t<TArgs>...> &p_job)
		{
			using DecayedProvider = Provider<std::decay_t<TArgs>...>;

			auto &base = _providers[p_event];
			if (base == nullptr)
			{
				base = std::make_unique<DecayedProvider>();
			}
			auto *typed = dynamic_cast<DecayedProvider *>(base.get());
			if (typed == nullptr)
			{
				throw std::runtime_error("Mismatched parameter types for event");
			}
			return typed->contractProvider.subscribe(p_job);
		}

		template <typename... TArgs>
		void _invalidate(const TType &p_event)
		{
			using DecayedProvider = Provider<std::decay_t<TArgs>...>;

			auto it = _providers.find(p_event);
			if (it != _providers.end())
			{
				if (auto *typed = dynamic_cast<DecayedProvider *>(it->second.get()))
				{
					typed->contractProvider.invalidateContracts();
				}
			}
		}

		template <typename F, std::size_t... I>
		auto _subscribeFromCallableImpl(const TType &p_event, F &&f, std::index_sequence<I...>)
		{
			using Tuple = typename FunctionTraits<std::decay_t<F>>::ArgsTuple;
			using JobT = Job<std::decay_t<std::tuple_element_t<I, Tuple>>...>;

			JobT job(std::forward<F>(f));
			return _subscribe<std::decay_t<std::tuple_element_t<I, Tuple>>...>(p_event, job);
		}

		template <typename F>
		auto _subscribeFromCallable(const TType &p_event, F &&f)
		{
			using Tuple = typename FunctionTraits<std::decay_t<F>>::ArgsTuple;
			return _subscribeFromCallableImpl(p_event, std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}

	public:
		template <typename... TArgs>
		static void emit(const TType &p_event, TArgs &&...p_args)
		{
			if (spk::Singleton<EventDispatcher<TType>>::instance() == nullptr)
			{
				spk::Singleton<EventDispatcher<TType>>::instanciate();
			}
			spk::Singleton<EventDispatcher<TType>>::instance()->template _emit<std::decay_t<TArgs>...>(p_event, std::forward<TArgs>(p_args)...);
		}

		template <typename... TArgs>
		static Contract<std::decay_t<TArgs>...> subscribe(const TType &p_event, const Job<std::decay_t<TArgs>...> &p_job)
		{
			if (spk::Singleton<EventDispatcher<TType>>::instance() == nullptr)
			{
				spk::Singleton<EventDispatcher<TType>>::instanciate();
			}
			return spk::Singleton<EventDispatcher<TType>>::instance()->template _subscribe<std::decay_t<TArgs>...>(p_event, p_job);
		}

		template <typename F>
		static auto subscribe(const TType &p_event, F &&f)
		{
			if (spk::Singleton<EventDispatcher<TType>>::instance() == nullptr)
			{
				spk::Singleton<EventDispatcher<TType>>::instanciate();
			}
			return spk::Singleton<EventDispatcher<TType>>::instance()->_subscribeFromCallable(p_event, std::forward<F>(f));
		}

		template <typename... TArgs>
		static void invalidateContracts(const TType &p_event)
		{
			if (spk::Singleton<EventDispatcher<TType>>::instance() != nullptr)
			{
				spk::Singleton<EventDispatcher<TType>>::instance()->template _invalidate<std::decay_t<TArgs>...>(p_event);
			}
		}
	};
}
