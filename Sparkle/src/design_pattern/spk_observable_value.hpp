#pragma once

#include "design_pattern/spk_notifier.hpp"

namespace spk
{
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