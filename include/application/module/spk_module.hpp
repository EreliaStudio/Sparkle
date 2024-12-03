#pragma once

#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/system/event/spk_event.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	/**
	 * @class IModule
	 * @brief Interface for modules that handle events in the system.
	 * 
	 * This interface defines methods for retrieving event IDs and handling events.
	 * Classes inheriting from `IModule` should implement these methods.
	 */
	class IModule
	{
	public:
		/**
		 * @brief Retrieves the event IDs this module listens to.
		 * 
		 * @return A vector of event IDs.
		 */
		virtual const std::vector<UINT>& eventIDs() = 0;

		/**
		 * @brief Processes an incoming event.
		 * 
		 * @param p_event The event to process.
		 */
		virtual void receiveEvent(spk::Event&& p_event) = 0;
	};

	/**
	 * @class Module
	 * @brief A templated module implementation for handling specific event types.
	 * 
	 * This class provides functionality for queuing and processing events of a specific type.
	 * Derived classes must implement the `_treatEvent` and `_convertEventToEventType` methods
	 * to define how events are treated and converted.
	 * 
	 * @tparam TEventType The specific event type this module handles.
	 */
	template <typename TEventType>
	class Module : public IModule
	{
	private:
		spk::ThreadSafeQueue<TEventType> _eventQueue; /**< @brief Thread-safe queue for storing events. */

		/**
		 * @brief Processes an individual event of the specific type.
		 * 
		 * Derived classes must implement this method to define how to handle the event.
		 * 
		 * @param p_event The event to process.
		 */
		virtual void _treatEvent(TEventType&& p_event) = 0;

		/**
		 * @brief Converts a generic event to the specific event type.
		 * 
		 * Derived classes must implement this method to define the conversion logic.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted event of the specific type.
		 */
		virtual TEventType _convertEventToEventType(spk::Event&& p_event) = 0;

		/**
		 * @brief Inserts an event into the queue.
		 * 
		 * @param p_event The event to insert.
		 */
		void _insertEvent(TEventType&& p_event)
		{
			_eventQueue.push(std::move(p_event));
		}

	public:
		/**
		 * @brief Constructs a `Module` object.
		 */
		Module() {}

		/**
		 * @brief Retrieves the event IDs this module listens to.
		 * 
		 * This method returns the static `EventIDs` vector from the event type.
		 * 
		 * @return A vector of event IDs.
		 */
		const std::vector<UINT>& eventIDs()
		{
			return TEventType::EventIDs;
		}

		/**
		 * @brief Processes an incoming event by converting and queuing it.
		 * 
		 * @param p_event The event to process.
		 */
		void receiveEvent(spk::Event&& p_event)
		{
			_insertEvent(_convertEventToEventType(std::move(p_event)));
		}

		/**
		 * @brief Processes all queued events.
		 * 
		 * This method dequeues all events and processes them using `_treatEvent`.
		 */
		void treatMessages()
		{
			while (_eventQueue.empty() == false)
			{
				_treatEvent(std::move(_eventQueue.pop()));
			}
		}
	};
}
