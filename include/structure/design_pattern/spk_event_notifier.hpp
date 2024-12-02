#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include <map>

namespace spk
{
	/**
	 * @class EventNotifier
	 * @brief A templated class to manage event-based notifications with subscriptions.
	 * 
	 * The `EventNotifier` class allows you to associate specific events with callback functions.
	 * Subscribers can register callbacks for specific events, and when the event is triggered,
	 * all associated callbacks are executed.
	 * 
	 * Example usage:
	 * @code
	 * spk::EventNotifier<std::string> notifier;
	 * 
	 * auto contract = notifier.subscribe("EventName", []() { std::cout << "Event triggered!" << std::endl; });
	 * notifier.notifyEvent("EventName"); // Outputs: Event triggered!
	 * @endcode
	 * 
	 * @tparam TType The type of the events managed by the `EventNotifier`.
	 */
	template<typename TType>
	class EventNotifier
	{
	public:
		using Contract = ContractProvider::Contract; /**< @brief Type alias for subscription contracts. */

	private:
		std::map<TType, ContractProvider> _eventProviders; /**< @brief Map of events to their associated `ContractProvider`s. */

	public:
		/**
		 * @brief Default constructor.
		 */
		EventNotifier() = default;

		/**
		 * @brief Destructor. Automatically cleans up resources.
		 */
		virtual ~EventNotifier() = default;

		/**
		 * @brief Subscribes a callback to a specific event.
		 * 
		 * The callback will be executed whenever the specified event is triggered.
		 * 
		 * @param p_event The event to subscribe to.
		 * @param p_job The callback function to associate with the event.
		 * @return A `Contract` object that manages the subscription lifecycle.
		 */
		ContractProvider::Contract subscribe(const TType& p_event, const ContractProvider::Job& p_job)
		{
			return _eventProviders[p_event].subscribe(p_job);
		}

		/**
		 * @brief Invalidates all subscriptions for a specific event.
		 * 
		 * This method removes all callbacks associated with the specified event.
		 * 
		 * @param p_event The event whose subscriptions should be invalidated.
		 */
		void invalidateContracts(const TType& p_event)
		{
			_eventProviders[p_event].invalidateContracts();
		}

		/**
		 * @brief Notifies all subscribers of a specific event.
		 * 
		 * Triggers all callbacks associated with the specified event.
		 * 
		 * @param p_event The event to notify.
		 */
		void notifyEvent(const TType& p_event)
		{
			auto it = _eventProviders.find(p_event);
			if (it != _eventProviders.end())
			{
				it->second.trigger();
			}
		}
	};
}
