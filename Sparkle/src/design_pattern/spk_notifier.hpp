#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace spk
{
    /**
	 * @class Notifier
     * @brief Manages a list of subscribers (via callbacks) and notifies them uppon request.
     * 
     * The Notifier class is designed to handle event notification to a set of subscribers. It allows
     * subscribers to register callback functions that should be called when a specific event occurs.
     * Subscribers receive a `Contract` object upon subscription, which can be used to manage their
     * subscription status, including cancellation, pausing, and resuming notifications.
     *
     * This class supports moving but not copying to ensure that the notifier and its subscriptions
     * remain uniquely associated with a specific context.
     *
     * Usage example:
     * @code
     * spk::Notifier notifier;
     * auto contract = notifier.subscribe([]{ std::cout << "Event occurred!" << std::endl; });
     * notifier.notify_all(); // Triggers the callback
     * @endcode
     *
     * @note The Notifier class is thread-safe and ensures that subscription management and event
     * notification can safely occur from multiple threads.
     */
    class Notifier
    {
    public:
        /**
         * @brief Describe the required callback type to provide when subscribing to a Notifier
        */
        using Callback = std::function<void()>;

        /**
	     * @class Contract
         * @brief Represents a subscription contract between a subscriber and a Notifier.
         * 
         * The Contract class encapsulates the relationship between a subscriber and the Notifier, providing
         * mechanisms to manage the subscription. It allows subscribers to cancel their subscription,
         * pause and resume notifications, and check their current subscription status.
         *
         * Upon subscription to a Notifier, subscribers receive a unique Contract object that must be kept
         * alive as long as the subscription should remain active. The Contract ensures that callbacks are
         * not called after it is canceled or while it is paused.
         *
         * Usage example:
         * @code
         * spk::Notifier notifier;
         * auto contract = notifier.subscribe([]{ std::cout << "Notification received." << std::endl; });
         * contract->pause(); // Temporarily stops notifications
         * contract->resume(); // Resumes notifications
         * contract->cancel(); // Cancels the subscription
         * @endcode
         *
         * @note The Contract class is designed to be used in conjunction with the Notifier class and is
         * thread-safe, allowing subscription management from multiple threads.
         * 
	     * @see Notifier
         */
        class Contract
        {
        public:
            /**
             * @brief Constructs a default Contract instance.
             */
            Contract();

            /**
             * @brief Destructs the Contract instance.
             */
            ~Contract();

            /**
             * @brief Assigns the state of another Contract to this one.
             * 
             * @param p_other The other Contract to copy state from.
             * @return A reference to this Contract after copying the state.
             */
            Contract& operator = (const Contract& p_other);

            /**
             * @brief Checks if the Contract is canceled.
             * 
             * @return True if the Contract has been canceled, false otherwise.
             */
            bool isCanceled() const;

            /**
             * @brief Cancels the Contract.
             * 
             * Once canceled, the Contract cannot be resumed or receive notifications.
             */
            void cancel();

            /**
             * @brief Checks if the Contract is paused.
             * 
             * @return True if the Contract is currently paused, false otherwise.
             */
            bool isPaused() const;

            /**
             * @brief Pauses the Contract.
             * 
             * A paused Contract temporarily stops receiving notifications until resumed.
             */
            void pause();

            /**
             * @brief Resumes a paused Contract.
             * 
             * Allows a previously paused Contract to start receiving notifications again.
             */
            void resume();

            /**
             * @brief Sends a notification using the Contract's callback.
             * 
             * This method is typically called by the Notifier to trigger the Contract's callback.
             */
            void notify();

        private:
            friend class Notifier;
            Contract(Notifier *p_subject, const Callback& p_callback);

            Notifier *_subject;
            Callback _callback;
            std::atomic_bool _isCanceled;
            std::atomic_bool _isPaused;
        };

        /**
         * @brief Constructs a Notifier instance.
         */
        Notifier();

        /**
         * @brief Destructs the Notifier instance.
         */
        ~Notifier();

        /**
         * @brief Deleted copy constructor.
         * 
         * Prevents copying of Notifier instances.
         */
        Notifier(const Notifier& p_other) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * 
         * Prevents assignment from another Notifier instance.
         */
        Notifier& operator = (const Notifier& p_other) = delete;

        /**
         * @brief Move constructor.
         * 
         * Enables the transfer of ownership of a Notifier instance from one object to another.
         * 
         * @param p_other The other Notifier instance to move from.
         */
        Notifier(Notifier&& p_other);

        /**
         * @brief Move assignment operator.
         * 
         * Allows the Notifier to acquire the state of another Notifier instance using move semantics.
         * 
         * @param p_other The other Notifier instance to move from.
         * @return A reference to this Notifier after acquiring the state.
         */
        Notifier& operator = (Notifier&& p_other);

        /**
         * @brief Subscribes to notifications with a specific callback.
         * 
         * @param p_callback The callback function to be called when a notification is sent.
         * @return A unique pointer to a Contract that manages the subscription.
         */
        std::unique_ptr<Contract> subscribe(const Callback& p_callback);

        /**
         * @brief Notifies all active Contracts.
         * 
         * Calls the callback of each active Contract subscribed to this Notifier.
         */
        void notify_all();

        /**
         * @brief Returns the number of active contracts.
         * 
         * This method provides the count of currently active contracts subscribed to the notifier.
         * It includes only active subscriptions.
         *
         * @return The number of active contracts.
         */
        size_t nbContracts() const;

    private:
        std::vector<Contract *> _contracts;
        std::vector<Contract *> _inactiveContracts;
        std::mutex _mu;

        std::atomic_bool _deactivated;

        friend class Contract;
        void unsubscribe(Contract *p_contract);
        void pause(Contract *p_contract);
        void resume(Contract *p_contract);
    };
}
