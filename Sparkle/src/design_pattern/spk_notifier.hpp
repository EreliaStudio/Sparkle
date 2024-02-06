#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace spk
{
  /**
   * @brief Notifier allows creating Contracts by subscribing a Callback.
   * @details Notifier and Contracts are thread-safe.
   * All contracts are cancelled if the Notifier is destroyed.
   */
    class Notifier
    {
    public:
        using Callback = std::function<void()>;

        /**
         * @brief The Contract represents the subscription to the Notifier.
         * @details A Contract is valid until it is cancelled. The Contract
         * cancels itself if it is destroyed.
         * All of its methods are guaranteed to be safe even after the Notifier
         * has been destroyed.
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
