#pragma once

#include "design_pattern/spk_notifier.hpp"

namespace spk
{
    /**
     * @class ObservableValue
     *
     * @tparam TType The type of the value being observed.
     *
     * @brief A template class for creating observable values that notify subscribers when changed.
     *
     * The ObservableValue class wraps a value of a specified type and allows subscribers to be notified
     * whenever the value changes. It leverages the Notifier class to manage subscriptions and notifications,
     * making it ideal for implementing the observer design pattern in a type-safe manner.
     *
     * Subscribers can register callback functions that will be executed whenever the value is modified.
     * Modifications to the value can be made in a way that either triggers notifications or not, providing
     * flexibility in how and when observers are informed of changes.
     *
     * This class supports basic arithmetic operations that automatically notify all subscribers upon changes,
     * making it particularly useful for data binding scenarios where changes to the data need to be propagated
     * to interested parties, such as UI elements or logging mechanisms.
     *
     * Usage example:
     * @code
     * spk::ObservableValue<int> observableInt(10);
     * auto contract = observableInt.subscribe([]{ std::cout << "Value changed!" << std::endl; });
     * observableInt = 20; // Subscribers will be notified of this change
     * @endcode
     *
     * @see Notifier
     */
    template <typename TType>
    class ObservableValue
    {
    public:
        /**
         * @brief Describe the required callback type to provide when subscribing to a ObservableValue
        */
        using Contract = Notifier::Contract;

        /**
         * @brief Constructs an ObservableValue with the specified initial value.
         *
         * Allows for direct initialization of the ObservableValue's internal value using the provided arguments.
         * This constructor is useful for types that require multiple arguments for construction.
         *
         * @tparam Args Variadic template parameter to accept multiple arguments.
         * @param p_args Arguments to forward to the constructor of the TType.
         */
        template <typename... Args>
        explicit ObservableValue(Args &&...p_args) :
            _value(std::forward<Args>(p_args)...)
        {

        }

        /**
         * @brief Constructs an ObservableValue with an optional initial value.
         *
         * Initializes the internal value to the specified value, or to the default value of TType if none is provided.
         *
         * @param p_value The initial value to set, defaulting to TType's default value.
         */
        explicit ObservableValue(const TType &p_value = TType()) :
            _value(p_value)
        {

        }

        /**
         * @brief Sets the value without notifying subscribers.
         *
         * Updates the internal value to the specified value without triggering any notifications to the subscribers.
         *
         * @param p_value The new value to set.
         */
        void setWithoutNotification(const TType &p_value)
        {
            _value = p_value;
        }

        /**
         * @brief Implicit conversion operator to TType.
         *
         * Allows the ObservableValue to be used in contexts where a TType is expected, providing direct access to the internal value.
         *
         * @return The current value of the ObservableValue.
         */
        operator TType() const
        {
            return _value;
        }

        /**
         * @brief Assigns a new value to the ObservableValue and notifies subscribers.
         *
         * Sets the internal value to the specified value and triggers notifications to all subscribers.
         *
         * @param p_value The new value to assign.
         * @return A reference to this ObservableValue instance.
         */
        ObservableValue &operator=(const TType &p_value)
        {
            _value = p_value;
            notify_all();
            return *this;
        }

        /**
         * @brief Adds the specified value to the internal value and notifies subscribers.
         *
         * Performs an addition assignment on the internal value and triggers notifications to all subscribers.
         *
         * @param p_value The value to add.
         * @return A reference to this ObservableValue instance.
         */
        ObservableValue &operator+=(const TType &p_value)
        {
            _value += p_value;
            notify_all();
            return *this;
        }

        /**
         * @brief Subtracts the specified value from the internal value and notifies subscribers.
         *
         * Performs a subtraction assignment on the internal value and triggers notifications to all subscribers.
         *
         * @param p_value The value to subtract.
         * @return A reference to this ObservableValue instance.
         */
        ObservableValue &operator-=(const TType &p_value)
        {
            _value -= p_value;
            notify_all();
            return *this;
        }

        /**
         * @brief Multiplies the internal value by the specified value and notifies subscribers.
         *
         * Performs a multiplication assignment on the internal value and triggers notifications to all subscribers.
         *
         * @param p_value The value to multiply by.
         * @return A reference to this ObservableValue instance.
         */
        ObservableValue &operator*=(const TType &p_value)
        {
            _value *= p_value;
            notify_all();
            return *this;
        }

        /**
         * @brief Divides the internal value by the specified value and notifies subscribers.
         *
         * Performs a division assignment on the internal value and triggers notifications to all subscribers.
         *
         * @param p_value The divisor.
         * @return A reference to this ObservableValue instance.
         */
        ObservableValue &operator/=(const TType &p_value)
        {
            _value /= p_value;
            notify_all();
            return *this;
        }

        /**
         * @brief Subscribes a callback to be notified of changes to the ObservableValue.
         *
         * Registers a callback function that will be called whenever the ObservableValue is modified.
         *
         * @param callback The callback function to register.
         * @return A unique_ptr to a Contract, managing the subscription lifecycle.
         */
        std::unique_ptr<Contract> subscribe(const typename Notifier::Callback &callback)
        {
            return std::move(_notifier.subscribe(callback));
        }

        /**
         * @brief Provides mutable access to the internal value.
         *
         * @return A reference to the internal value.
         */
        TType &get()
        {
            return _value;
        }

        /**
         * @brief Provides immutable access to the internal value.
         *
         * @return A const reference to the internal value.
         */
        const TType &get() const
        {
            return _value;
        }

        /**
         * @brief Notifies all subscribers of a change to the ObservableValue.
         *
         * This function triggers the registered callbacks, indicating that the ObservableValue has been modified.
         */
        void notify_all()
        {
            _notifier.notify_all();
        }

        size_t nbContracts() const
        {
            return (_notifier.nbContracts());
        }

    private:
        TType _value;
        Notifier _notifier;
    };
}