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
            Contract();
            ~Contract();

            Contract& operator = (const Contract& p_other);

            bool isCanceled() const;
            void cancel();

            bool isPaused() const;
            void pause();
            void resume();
            
            void notify();

        private:
            friend class Notifier;
            Contract(Notifier *p_subject, const Callback& p_callback);

            Notifier *_subject;
            Callback _callback;
            std::atomic_bool _isCanceled;
            std::atomic_bool _isPaused;
        };

        Notifier();
        ~Notifier();

        Notifier(const Notifier& p_other) = delete;
        Notifier operator = (const Notifier& p_other) = delete;

        Notifier(Notifier&& p_other);
        Notifier& operator = (Notifier&& p_other);

        std::unique_ptr<Contract> subscribe(const Callback& p_callback);
        void notify_all();

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
