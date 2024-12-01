#pragma once

#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/system/event/spk_event.hpp"

#include "widget/spk_widget.hpp"

namespace spk
{
	class IModule
	{
	public:
		virtual const std::vector<UINT>& eventIDs() = 0;
		virtual void receiveEvent(spk::Event&& p_event) = 0;
	};

	template <typename TEventType>
	class Module : public IModule
	{
	private:
		spk::ThreadSafeQueue<TEventType> _eventQueue;

		virtual void _treatEvent(TEventType&& p_event) = 0;
		virtual TEventType _convertEventToEventType(spk::Event&& p_event) = 0;

		void _insertEvent(TEventType&& p_event)
		{
			_eventQueue.push(std::move(p_event));
		}


	public:
		Module()
		{

		}

		const std::vector<UINT>& eventIDs()
		{
			return (TEventType::EventIDs);
		}

		void receiveEvent(spk::Event&& p_event)
		{
			_insertEvent(_convertEventToEventType(std::move(p_event)));
		}

		void treatMessages()
		{
			while (_eventQueue.empty() == false)
			{
				_treatEvent(std::move(_eventQueue.pop()));
			}
		}
	};
}