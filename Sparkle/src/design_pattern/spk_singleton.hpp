#pragma once 

#include <memory>
#include <stdexcept>

#include "spk_basic_functions.hpp"

namespace spk
{
	/**
     * @class Singleton
     * @brief Template class for implementing the singleton design pattern.
     *
     * This class template is used to create singleton classes, ensuring that only one instance
     * of the class can exist at any time. It provides methods for instantiating, accessing,
     * and releasing the singleton instance. The Singleton class uses std::unique_ptr to manage
     * the singleton instance, providing automatic cleanup when the program exits or when release()
     * is explicitly called.
     *
     * Usage:
     * @code
     * class MySingletonClass : public spk::Singleton<MySingletonClass>
     * {
     *     friend class Singleton<MySingletonClass>; // Allow Singleton to access the constructor
     * 
     * private:
     *     MySingletonClass() {} // Private constructor
     * 
     * public:
     *     void doSomething() {}
     * };
     * 
     * // Instantiate the singleton
     * MySingletonClass::instanciate();
     * 
     * // Access the singleton instance
     * MySingletonClass::instance()->doSomething();
     * 
     * // Release the singleton instance
     * MySingletonClass::release();
     * @endcode
     *
     * @tparam TType The type of the singleton class.
     */
	template <typename TType>
	class Singleton
	{
    public:
        using Type = TType; //!< A redirection to the type stored inside the singleton

		/**
		 * @class Instanciator
		 * @brief Utility class to manage the lifecycle of singleton instances within scope.
		 *
		 * This class is designed to automatically manage the references to the singleton instance.
		 * It increments a reference count upon construction and decrements it upon destruction.
		 * When the reference count reaches zero, the singleton instance is released.
		 */
		class Instanciator
		{
		private:
			static inline size_t reference = 0; //!< Static count of active references to the singleton.

		public:
			/**
			 * @brief Constructor that instantiates the singleton if it is the first reference.
			 *
			 * @tparam Args Variadic template parameters to forward to the singleton's constructor.
			 * @param p_args Arguments to pass to the singleton's constructor.
			 */
			template <typename... Args>
			Instanciator(Args &&...p_args)
			{
				if (reference == 0)
					Singleton<TType>::instanciate(std::forward<Args>(p_args)...);
				reference++;
			}

			/**
			 * @brief Destructor that releases the singleton instance if this was the last reference.
			 */
			~Instanciator()
			{
				reference--;
				if (reference == 0)
					Singleton<TType>::release();
			}

			/**
			 * @brief Overloads the arrow operator to provide direct access to the singleton instance.
			 *
			 * @return A pointer to the singleton instance.
			 */
			std::unique_ptr<TType>& operator ->()
			{
				return (Singleton<TType>::instance());
			}
		};
          
	protected:
		/**
		 * @brief Protected constructor.
		 * 
		 * Prevents direct creation of the singleton instance from outside the class, ensuring the singleton pattern is enforced.
		 */
		Singleton()
		{
			
		}

		/**
		 * @brief Static unique pointer to the singleton instance.
		 * 
		 * This pointer manages the lifetime of the singleton instance. It ensures that there is only one instance of the
		 * singleton class at any time and provides automatic cleanup.
		 */
		static inline std::unique_ptr<TType> _instance = nullptr;

	public:
		/**
         * @brief Instantiates the singleton instance with the provided arguments.
         *
         * This method creates a new instance of the singleton class with the given arguments if it has not
         * already been instantiated. If the instance already exists, it throws a std::runtime_error.
         *
         * @tparam Args Variadic template arguments to forward to the constructor of TType.
         * @param p_args Arguments to forward to the constructor of TType.
         * @return A reference to the std::unique_ptr managing the singleton instance.
         * @throws std::runtime_error if the singleton instance already exists.
         */
		template <typename... Args>
		static std::unique_ptr<TType>& instanciate(Args &&...p_args)
		{
			if (_instance != nullptr)
				spk::throwException("Can't instanciate an already instancied singleton");
			
			_instance = std::unique_ptr<TType>(new TType(std::forward<Args>(p_args)...));
			return (_instance);
		}

		/**
         * @brief Returns a reference to the singleton instance.
         *
         * @return A reference to the std::unique_ptr managing the singleton instance.
         */
		static std::unique_ptr<TType>& instance()
		{
			return (_instance);
		}

		/**
         * @brief Returns a const reference to the singleton instance.
         *
         * @return A const reference to the std::unique_ptr managing the singleton instance.
         */
		static const std::unique_ptr<const TType>& c_instance()
		{
			return _instance;
		}
		
        /**
         * @brief Releases the singleton instance.
         *
         * This method destroys the instance of the singleton class and resets the unique pointer.
         */
		static void release()
		{
			_instance.reset();
		}
	};
}