#pragma once

#include <iostream>

namespace spk
{
	template <typename TType>
	class SafePointer
	{
	private:
		TType* _ptr = nullptr;

	public:
		SafePointer(TType* p_ptr = nullptr) : _ptr(p_ptr) {}
		
		template <typename SmartPointer,
			typename = std::enable_if_t<
			std::is_pointer_v<decltype(std::declval<SmartPointer>().get())>>>
			SafePointer(SmartPointer&& p_smartPointer) : _ptr(reinterpret_cast<TType*>(p_smartPointer.get()))
		{

		}

		~SafePointer() = default;
		SafePointer(const SafePointer&) = default;
		SafePointer& operator=(const SafePointer&) = default;

		operator TType* () const
		{
			return (_ptr);
		}

		operator SafePointer<const TType>() const
		{
			return (SafePointer<const TType>(_ptr));
		}

		TType& operator*() const { return (*_ptr); }
		TType* operator->() const { return (_ptr); }
		TType* get() const { return (_ptr); }

		friend std::ostream& operator << (std::ostream& p_os, const spk::SafePointer<TType>& p_ptr)
		{
			p_os << p_ptr.get();
			return (p_os);
		}

		friend std::wostream& operator << (std::wostream& p_os, const spk::SafePointer<TType>& p_ptr)
		{
			p_os << p_ptr.get();
			return (p_os);
		}
	};
}