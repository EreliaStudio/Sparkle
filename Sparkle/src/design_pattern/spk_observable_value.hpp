#pragma once

#include "design_pattern/spk_notifier.hpp"

namespace spk
{
    /**
	 * @class ObservableValue<TType>
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
     * @tparam TType The type of the value being observed.
     * 
     * @see Notifier 
     */
    template<typename TType>
    class ObservableValue
    {
    public:
        using Contract = Notifier::Contract;

        template <typename... Args>
        ObservableValue(Args&&... p_args) : _value(std::forward<Args>(p_args)...)
        {

        }

        ObservableValue(const TType& p_value = TType()) : _value(p_value)
        {

        }

        void setWithoutNotification(const TType& p_value)
        {
            _value = p_value;
        }

        operator TType() const
        {
            return _value;
        }

        ObservableValue& operator=(const TType& p_value)
        {
            _value = p_value;
            notify_all();
            return *this;
        }

        ObservableValue& operator+=(const TType& p_value)
        {
            _value += p_value;
            notify_all();
            return *this;
        }

        ObservableValue& operator-=(const TType& p_value)
        {
            _value -= p_value;
            notify_all();
            return *this;
        }

        ObservableValue& operator*=(const TType& p_value)
        {
            _value *= p_value;
            notify_all();
            return *this;
        }

        ObservableValue& operator/=(const TType& p_value)
        {
            _value /= p_value;
            notify_all();
            return *this;
        }

        std::unique_ptr<Contract> subscribe(const typename Notifier::Callback& callback)
        {
            return _notifier.subscribe(callback);
        }

        TType& get()
        {
            return _value;
        }

        const TType& get() const
        {
            return _value;
        }

        void notify_all()
        {
            _notifier.notify_all();
        }

    private:
        TType _value;
        Notifier _notifier;
    };
}