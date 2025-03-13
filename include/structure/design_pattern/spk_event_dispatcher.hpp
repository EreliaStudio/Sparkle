#pragma once

#include "structure/design_pattern/spk_observer.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include <unordered_map>

namespace spk
{
	template <typename TType, typename... TArgs>
	class EventDispatcher : public spk::Singleton<EventDispatcher<TType, TArgs...>>
	{
		friend class spk::Singleton<EventDispatcher<TType, TArgs...>>;

	public:
		using Contract = typename Observer<TType, TArgs...>::Contract;
		using Job = typename Observer<TType, TArgs...>::Job;

	private:
		Observer<TType, TArgs...> _eventNotifier;

		void _emit(const TType &p_event, TArgs... args)
		{
			_eventNotifier.notifyEvent(p_event, args...);
		}

		Contract _subscribe(const TType &p_event, const Job &p_job)
		{
			return _eventNotifier.subscribe(p_event, p_job);
		}

		EventDispatcher() = default;

	public:
		static void emit(const TType &p_event, TArgs... args)
		{
			if (spk::Singleton<EventDispatcher<TType, TArgs...>>::instance() == nullptr)
			{
				spk::Singleton<EventDispatcher<TType, TArgs...>>::instanciate();
			}
			spk::Singleton<EventDispatcher<TType, TArgs...>>::instance()->_emit(p_event, args...);
		}

		static Contract subscribe(const TType &p_event, const Job &p_job)
		{
			if (spk::Singleton<EventDispatcher<TType, TArgs...>>::instance() == nullptr)
			{
				spk::Singleton<EventDispatcher<TType, TArgs...>>::instanciate();
			}
			return spk::Singleton<EventDispatcher<TType, TArgs...>>::instance()->_subscribe(p_event, p_job);
		}
	};
}