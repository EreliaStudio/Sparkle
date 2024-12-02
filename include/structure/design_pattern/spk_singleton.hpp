#pragma once 

#include <memory>
#include <stdexcept>

#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	/**
	 * @class Singleton
	 * @brief A template class for implementing the Singleton design pattern with safe instance management.
	 * 
	 * The `Singleton` class ensures that only one instance of a type exists in the program. It provides
	 * mechanisms for controlled instantiation and destruction, along with thread-safe access using 
	 * `SafePointer`. The `Instanciator` class is a utility for managing reference counting to ensure 
	 * proper cleanup when the instance is no longer needed.
	 * 
	 * Example usage:
	 * @code
	 * class MySingleton : public spk::Singleton<MySingleton>
	 * {
	 *	 friend class spk::Singleton<MySingleton>;
	 * private:
	 *	 MySingleton() {}
	 * public:
	 *	 void doSomething() { std::cout << "Singleton instance working!" << std::endl; }
	 * };
	 * 
	 * spk::Singleton<MySingleton>::instanciate();
	 * spk::Singleton<MySingleton>::instance()->doSomething();
	 * spk::Singleton<MySingleton>::release();
	 * @endcode
	 * 
	 * @tparam TType The type of the singleton instance.
	 */
	template <typename TType>
	class Singleton
	{
	public:
		using Type = TType; /**< @brief Alias for the singleton type. */

		/**
		 * @class Instanciator
		 * @brief Manages references to the singleton instance, ensuring proper cleanup when no references remain.
		 * 
		 * The `Instanciator` class handles reference counting for the singleton instance. When the last
		 * `Instanciator` is destroyed, the singleton instance is released automatically.
		 * 
		 * This class utility is to allow user to add the instanciation of the singleton as aggregation or composition
		 * inside user's classes, to manage its allocation and destruction directly with the object life cycle.
		 */
		class Instanciator
		{
		private:
			static inline size_t reference = 0; /**< @brief Tracks the number of active references. */

		public:
			/**
			 * @brief Constructs an `Instanciator`, instantiating the singleton if no other references exist.
			 * 
			 * @tparam Args Parameter pack for the singleton constructor.
			 * @param p_args Arguments to forward to the singleton constructor.
			 */
			template <typename... Args>
			Instanciator(Args &&...p_args)
			{
				if (reference == 0)
					Singleton<TType>::instanciate(std::forward<Args>(p_args)...);
				reference++;
			}

			/**
			 * @brief Destroys the `Instanciator`, releasing the singleton if this is the last reference.
			 */
			~Instanciator()
			{
				reference--;
				if (reference == 0)
					Singleton<TType>::release();
			}

			/**
			 * @brief Provides access to the singleton instance.
			 * 
			 * @return A `SafePointer` to the singleton instance.
			 */
			spk::SafePointer<TType> operator ->()
			{
				return (Singleton<TType>::instance());
			}
		};

	protected:
		/**
		 * @brief Protected default constructor to prevent external instantiation.
		 */
		Singleton() {}

		static inline TType* _instance = nullptr; /**< @brief Pointer to the singleton instance. */

	public:
		/**
		 * @brief Instantiates the singleton instance if it is not already instantiated.
		 * 
		 * @tparam Args Parameter pack for the singleton constructor.
		 * @param p_args Arguments to forward to the singleton constructor.
		 * @return A `SafePointer` to the singleton instance.
		 * @throws std::runtime_error If the singleton is already instantiated.
		 */
		template <typename... Args>
		static spk::SafePointer<TType> instanciate(Args &&...p_args)
		{
			if (_instance != nullptr)
				throw std::runtime_error("Can't instanciate an already instancied singleton");

			_instance = new TType(std::forward<Args>(p_args)...);
			return (_instance);
		}

		/**
		 * @brief Retrieves a safe pointer to the singleton instance.
		 * 
		 * @return A `SafePointer` to the singleton instance.
		 */
		static spk::SafePointer<TType> instance()
		{
			return (_instance);
		}

		/**
		 * @brief Retrieves a constant safe pointer to the singleton instance.
		 * 
		 * @return A `SafePointer` to the singleton instance as `const`.
		 */
		static spk::SafePointer<const TType> c_instance()
		{
			return _instance;
		}

		/**
		 * @brief Releases the singleton instance, deleting the underlying object.
		 */
		static void release()
		{
			delete _instance;
			_instance = nullptr;
		}
	};
}
