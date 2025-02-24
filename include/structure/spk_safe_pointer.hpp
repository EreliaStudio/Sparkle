#pragma once

#include <type_traits>
#include <iostream>

namespace spk
{
    template <typename TType>
    class SafePointer
    {
    private:
        TType* _ptr = nullptr;

    public:
        SafePointer(TType* p_ptr = nullptr) :
			_ptr(p_ptr)
        {
        }

		template <typename U>
		SafePointer(const SafePointer<U>& p_other) :
			_ptr(dynamic_cast<TType*>(p_other.get()))
		{
		}

        SafePointer(const SafePointer&) = default;
        SafePointer& operator=(const SafePointer&) = default;

        ~SafePointer() = default;

        operator TType* () const
        {
            return _ptr;
        }

        operator SafePointer<const TType>() const
        {
            return SafePointer<const TType>(_ptr);
        }

        TType& operator*() const
        {
            return *_ptr;
        }

        TType* operator->() const
        {
            return _ptr;
        }

        TType* get() const
        {
            return _ptr;
        }

        explicit operator bool() const
        {
            return (_ptr != nullptr);
        }

        friend std::ostream& operator<<(std::ostream& os, const SafePointer<TType>& p_ptr)
        {
            os << p_ptr.get();
            return os;
        }

        friend std::wostream& operator<<(std::wostream& wos, const SafePointer<TType>& p_ptr)
        {
            wos << p_ptr.get();
            return wos;
        }
    };

    template <class To, class From>
    SafePointer<To>& safe_pointer_cast(SafePointer<From>& from)
    {
		static_assert(std::is_base_of<From, To>::value || std::is_base_of<To, From>::value, 
                      "safe_pointer_cast error: 'From' must be derived from 'To'");
		return *(reinterpret_cast<SafePointer<To>*>(&from));
    }

    template <class To, class From>
    const SafePointer<To>& safe_pointer_cast(const SafePointer<From>& from)
    {
		static_assert(std::is_base_of<From, To>::value || std::is_base_of<To, From>::value, 
                      "safe_pointer_cast error: 'From' must be derived from 'To'");

		return *(reinterpret_cast<const SafePointer<To>*>(&from));
    }
}
