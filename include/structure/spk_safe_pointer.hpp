#pragma once

#include <iostream>

namespace spk
{
	/**
	 * @class SafePointer
	 * @brief A lightweight wrapper around raw pointers to deny user from releasing memory.
	 * 
	 * The `SafePointer` class provides a wrapper around raw pointers, enabling conversions
	 * from smart pointers and integration with standard stream operators. This utility class
	 * is designed to allow user to retrieve pointer but without giving them the possibility
	 * to release the allocated memory.
	 * 
	 * Example usage:
	 * @code
	 * int value = 42;
	 * spk::SafePointer<int> safePtr(&value);
	 * 
	 * std::cout << *safePtr << std::endl; // Outputs: 42
	 * @endcode
	 * 
	 * @tparam TType The type of the object being pointed to.
	 */
	template <typename TType>
	class SafePointer
	{
	private:
		TType* _ptr = nullptr; /**< @brief The raw pointer being wrapped. */

	public:
		/**
		 * @brief Constructs a `SafePointer` from a raw pointer.
		 * 
		 * @param p_ptr The raw pointer to wrap.
		 */
		SafePointer(TType* p_ptr = nullptr) : _ptr(p_ptr) {}

		/**
		 * @brief Constructs a `SafePointer` from a smart pointer.
		 * 
		 * This constructor enables seamless conversion from smart pointers (e.g., `std::unique_ptr`, `std::shared_ptr`).
		 * 
		 * @tparam SmartPointer The type of the smart pointer.
		 * @param p_smartPointer The smart pointer to wrap.
		 */
		template <typename SmartPointer,
			typename = std::enable_if_t<
				std::is_pointer_v<decltype(std::declval<SmartPointer>().get())>>>
		SafePointer(SmartPointer&& p_smartPointer) : _ptr(p_smartPointer.get())
		{
		}

		/**
		 * @brief Default destructor.
		 */
		~SafePointer() = default;

		SafePointer(const SafePointer&) = default; /**< @brief Default copy constructor. */
		SafePointer& operator=(const SafePointer&) = default; /**< @brief Default copy assignment operator. */

		/**
		 * @brief Implicit conversion operator to the raw pointer.
		 * 
		 * @return The raw pointer.
		 */
		operator TType* () const
		{
			return _ptr;
		}

		/**
		 * @brief Implicit conversion to `SafePointer<const TType>`.
		 * 
		 * Allows seamless conversion from `SafePointer<TType>` to `SafePointer<const TType>`.
		 * 
		 * @return A `SafePointer` to a `const` version of the object.
		 */
		operator SafePointer<const TType>() const
		{
			return SafePointer<const TType>(_ptr);
		}

		/**
		 * @brief Dereferences the pointer.
		 * 
		 * Provides access to the object being pointed to.
		 * 
		 * @return A reference to the object being pointed to.
		 */
		TType& operator*() const { return *_ptr; }

		/**
		 * @brief Member access operator.
		 * 
		 * Provides access to the members of the object being pointed to.
		 * 
		 * @return The raw pointer.
		 */
		TType* operator->() const { return _ptr; }

		/**
		 * @brief Retrieves the raw pointer.
		 * 
		 * @return The raw pointer.
		 */
		TType* get() const { return _ptr; }

		/**
		 * @brief Stream insertion operator for `std::ostream`.
		 * 
		 * Outputs the raw pointer to the provided stream.
		 * 
		 * @param p_os The output stream.
		 * @param p_ptr The `SafePointer` to insert.
		 * @return A reference to the output stream.
		 */
		friend std::ostream& operator<<(std::ostream& p_os, const spk::SafePointer<TType>& p_ptr)
		{
			p_os << p_ptr.get();
			return p_os;
		}

		/**
		 * @brief Stream insertion operator for `std::wostream`.
		 * 
		 * Outputs the raw pointer to the provided wide-character stream.
		 * 
		 * @param p_os The wide-character output stream.
		 * @param p_ptr The `SafePointer` to insert.
		 * @return A reference to the wide-character output stream.
		 */
		friend std::wostream& operator<<(std::wostream& p_os, const spk::SafePointer<TType>& p_ptr)
		{
			p_os << p_ptr.get();
			return p_os;
		}
	};
}
