#pragma once 

#include <memory>
#include <stdexcept>

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
	protected:
		Singleton()
		{
		}

		static inline std::unique_ptr<TType> _instance = nullptr;

	public:
		/**
         * @brief Instantiates the singleton instance with the provided arguments.
         *
         * This method creates a new instance of the singleton class with the given arguments if it has not
         * already been instantiated. If the instance already exists, it throws a std::runtime_error.
         *
         * @tparam Args Variadic template arguments to forward to the constructor of TType.
         * @param args Arguments to forward to the constructor of TType.
         * @return A reference to the std::unique_ptr managing the singleton instance.
         * @throws std::runtime_error if the singleton instance already exists.
         */
		template <typename... Args>
		static std::unique_ptr<TType>& instanciate(Args &&...args)
		{
			if (_instance != nullptr)
				throw std::runtime_error("Can't instanciate an already instancied singleton");
			
			_instance = std::unique_ptr<TType>(new TType(std::forward<Args>(args)...));
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