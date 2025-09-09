#pragma once

#include <iostream>
#include <type_traits>

namespace spk
{
	template <typename TType>
	class SafePointer
	{
	public:
		using Type = TType;

	private:
		TType *_ptr = nullptr;

	public:
		SafePointer(TType *p_ptr = nullptr) :
			_ptr(p_ptr)
		{
		}

		template <typename ToType>
		SafePointer(const SafePointer<ToType> &p_other) :
			_ptr(dynamic_cast<TType *>(p_other.get()))
		{
			static_assert(std::is_base_of<ToType, TType>::value || std::is_base_of<TType, ToType>::value,
						  "SafePointer conversion error: TType and ToType must have an inheritance relationship.");
		}

		SafePointer(const SafePointer &) = default;
		SafePointer &operator=(const SafePointer &) = default;

		~SafePointer() = default;

		operator TType *() const
		{
			return _ptr;
		}

		operator SafePointer<const TType>() const
		{
			return SafePointer<const TType>(_ptr);
		}

		TType &operator*() const
		{
			return *_ptr;
		}

		TType *operator->() const
		{
			return _ptr;
		}

		TType *get() const
		{
			return _ptr;
		}

		explicit operator bool() const
		{
			return (_ptr != nullptr);
		}

		template <typename ToType>
		SafePointer<ToType> &upCast()
		{
			static_assert(std::is_base_of<TType, ToType>::value, "upCast error: ToType must be derived from TType.");

			return *reinterpret_cast<SafePointer<ToType> *>(this);
		}

		template <typename ToType>
		const SafePointer<ToType> &upCast() const
		{
			static_assert(std::is_base_of<TType, ToType>::value, "upCast error: ToType must be derived from TType.");

			return *reinterpret_cast<const SafePointer<ToType> *>(this);
		}

		template <typename ToType>
		SafePointer<ToType> &downCast()
		{
			static_assert(std::is_base_of<ToType, TType>::value, "downCast error: TType must be derived from ToType.");
			return *reinterpret_cast<SafePointer<ToType> *>(this);
		}

		template <typename ToType>
		const SafePointer<ToType> &downCast() const
		{
			static_assert(std::is_base_of<ToType, TType>::value, "downCast error: TType must be derived from ToType.");
			return *reinterpret_cast<const SafePointer<ToType> *>(this);
		}

		friend std::ostream &operator<<(std::ostream &p_os, const SafePointer<TType> &p_ptr)
		{
			p_os << p_ptr.get();
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const SafePointer<TType> &p_ptr)
		{
			p_wos << p_ptr.get();
			return p_wos;
		}
	};
}
