#pragma once

#include <map>
#include <memory>
#include "design_pattern/spk_notifier.hpp"

namespace spk
{
	/**
	 * @class EventManager
	 * 
	 * @tparam TEventType The type of the events. This type should be capable of being used as an index in a vector.
	 * 
	 * @brief Manages event subscriptions and notifications.
	 * 
	 * The EventManager class is a generic event handler that allows objects to subscribe to events of a specific type
	 * and get notified when those events occur. It uses the spk::Notifier class to manage subscriptions and notifications
	 * for each event type.
	 * 
	 * Usage example:
	 * @code
	 * spk::EventManager<MyEventType> eventManager;
	 * auto contract = eventManager.subscribe(MyEventType::Event1, []() { ... });
	 * eventManager.notify(MyEventType::Event1);
	 * @endcode
	 */
	template <typename TEventType>
	class EventManager
	{
	public:
		/**
		 * @brief Describe the required callback type to provide when subscribing to a Notifier
		*/
		using Callback = spk::Notifier::Callback;
		
		/**
		 * @brief Describe the required callback type to provide when subscribing to a ObservableValue
		*/
		using Contract = spk::Notifier::Contract;

	private:
		std::unordered_map<TEventType, spk::Notifier> _notifiers;

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new instance of the EventManager class.
		 */
		EventManager()
		{

		}

		/**
		 * @brief Subscribes to an event with a callback.
		 * 
		 * Registers a callback to be invoked when a specific event occurs. Returns a unique_ptr to a Contract
		 * that manages the lifetime of the subscription. When the Contract is destroyed, the subscription is automatically removed.
		 * 
		 * @param p_event The event to subscribe to.
		 * @param p_callback The callback to invoke when the event occurs.
		 * 
		 * @return std::unique_ptr<Contract> A unique pointer to a contract that controls the subscription.
		 */
		std::unique_ptr<Contract> subscribe(const TEventType& p_event, const Callback& p_callback)
		{
			return _notifiers[p_event].subscribe(p_callback);
		}

		/**
		 * @brief Notifies all subscribers of an event.
		 * 
		 * Triggers the event by invoking all registered callbacks for the specified event type.
		 * 
		 * @param p_event The event to notify.
		 */
		void notify_all(const TEventType& p_event)
		{
			_notifiers[p_event].notify_all();
		}
	};
}