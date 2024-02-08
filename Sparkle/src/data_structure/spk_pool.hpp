#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <mutex>

namespace spk
{
    /**
	 * @class Pool
     *
     * @tparam TType The type of objects to be managed by the pool.
     * 
     * @brief A template class for managing a pool of reusable objects of type TType.
     * 
     * The Pool class is designed to optimize resource allocation and reusability by maintaining
     * a pool of preallocated objects of a specified type. It allows for efficient reuse of objects,
     * reducing the overhead associated with frequent creation and destruction of objects.
     *
     * Objects are managed using `std::unique_ptr` with a custom deleter as, ensuring that objects are
     * automatically returned to the pool when they go out of scope or are otherwise no longer needed.
     * This mechanism simplifies memory management and helps to prevent memory leaks.
     *
     * The class uses a `std::recursive_mutex` to ensure thread safety, making it suitable for use
     * in multithreaded applications where objects from the pool may be obtained or returned by
     * multiple threads concurrently.
     *
     * Usage:
     * @code
     * spk::Pool<MyClass> myPool(10); // Create a pool with 10 pre-allocated objects
     * 
     * spk::Pool<MyClass>::Object myObject = myPool.obtain(); // Obtain an object from the pool
     * // Use myObject as needed; it will be automatically returned to the pool once out of scope
     * @endcode
     */
    template<typename TType>
    class Pool
	{
    private:    
        using Destructor = typename std::function<void(TType* p_toReturn)>;
    
    public:
        /**
         * @brief Describe the type of the provided object given by the pool uppon request
        */
        using Object = typename std::unique_ptr<TType, Destructor>;

    private:
        using Destructor = typename std::function<void(TType* p_toReturn)>;
        std::recursive_mutex _mutex;
        std::deque<std::unique_ptr<TType>> _preallocatedElements;

        const Destructor _destructorLambda = [&](TType* p_toReturn){ _release(p_toReturn); };

        void _release(TType* p_toReturn)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _preallocatedElements.push_back(std::unique_ptr<TType>(p_toReturn));
        }

    public:
        /**
         * @brief Constructor
         * 
         * Initializes a new instance of the Pool class with a specified initial pool size.
         * Preallocates a number of objects of type TType according to the provided size.
         * 
         * @param p_poolSize The initial size of the pool. Defaults to 0, resulting in an empty pool.
         */
        Pool(const size_t& p_poolSize = 0)
        {
            resize(p_poolSize);
        }

        /**
         * @brief Returns the current size of the pool.
         * 
         * Retrieves the number of preallocated objects currently available in the pool.
         * 
         * @return The size of the pool as a size_t value.
         */
        size_t size() const
        {
            return (_preallocatedElements.size());
        }

        /**
         * @brief Resizes the pool to a new size.
         * 
         * Adjusts the pool's size to hold a specified number of preallocated objects.
         * If the new size is larger, additional objects are preallocated. If the new size is
         * smaller, excess objects are destroyed.
         * 
         * @param p_newSize The new size of the pool.
         */
        void resize(size_t p_newSize)
		{
            std::lock_guard<std::recursive_mutex> lock(_mutex);

            while (_preallocatedElements.size() > p_newSize)
                _preallocatedElements.pop_back();

            while (_preallocatedElements.size() < p_newSize)
                _preallocatedElements.push_front(std::make_unique<TType>());
        }

        /**
         * @brief Obtains an object from the pool.
         * 
         * Provides an object from the pool. If the pool is empty, a new object is created,
         * added to the pool, and then provided. Objects are created and linked to it by the
         * pool to ensure they are returned to the pool automatically when no longer needed.
         * 
         * @return An Object of type TType.
         */
        Object obtain()
		{
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            
            if (_preallocatedElements.empty())
            {
                _preallocatedElements.push_front(std::make_unique<TType>());
            }

            Object item(_preallocatedElements.front().release(), _destructorLambda);

            _preallocatedElements.pop_front();

            return (std::move(item));
        }
    };
}
