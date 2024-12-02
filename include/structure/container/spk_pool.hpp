#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <mutex>

namespace spk
{
	/**
	 * @class Pool
	 * @brief A thread-safe object pool implementation for managing reusable objects of a specific type.
	 * 
	 * The `Pool` class allows for efficient reuse of objects by maintaining a pool of preallocated elements.
	 * It supports custom allocation and cleaning logic for objects, making it suitable for scenarios where
	 * object creation/destruction is expensive.
	 * 
	 * Example usage:
	 * @code
	 * spk::Pool<int> pool(
	 *	 []() { return new int(0); },  // Allocator
	 *	 [](int& obj) { obj = 0; }	// Cleaner
	 * );
	 * 
	 * auto obj = pool.obtain(); // Obtain an object from the pool
	 * *obj = 42;
	 * obj.reset(); // Return the object to the pool
	 * 
	 * {
	 *	 auto obj2 = pool.obtain(); // Obtain an object from the pool
	 *	 *obj2 = 12;
	 *	 // obj2 will be returned to the pool when quiting the scope
	 * }
	 * @endcode
	 * 
	 * @tparam TType The type of objects managed by the pool.
	 */
	template<typename TType>
	class Pool
	{
	public:
		using Allocator = typename std::function<TType* (void)>; /**< @brief Type definition for custom object allocator. */
		using Cleaner = typename std::function<void(TType&)>; /**< @brief Type definition for custom object cleaner. */

	private:
		using Destructor = typename std::function<void(TType* p_toReturn)>; /**< @brief Type definition for custom object destructor. */

	public:
		using Object = typename std::unique_ptr<TType, Destructor>; /**< @brief Managed object type returned by the pool. */

	private:
		std::recursive_mutex _mutex;
		Allocator _allocator;
		Cleaner _cleaner;
		std::deque<std::unique_ptr<TType>> _preallocatedElements;

		const Destructor _destructorLambda = [&](TType* p_toReturn) { _insert(p_toReturn); };

		void _insert(TType* p_toReturn)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_preallocatedElements.push_back(std::unique_ptr<TType>(p_toReturn));
		}

	public:
		/**
		 * @brief Default constructor. Initializes the pool with default allocator and cleaner.
		 */
		Pool() :
			_allocator([]() {return (new TType()); }),
			_cleaner([](TType& toClean) {})
		{

		}

		/**
		 * @brief Constructs a pool with custom allocator and cleaner functions.
		 * 
		 * @param p_allocator Custom function to allocate new objects.
		 * @param p_cleaner Custom function to clean objects before reuse.
		 */
		Pool(const Allocator& p_allocator, const Cleaner& p_cleaner) :
			_allocator(p_allocator),
			_cleaner(p_cleaner)
		{

		}

		/**
		 * @brief Updates the allocator function used by the pool.
		 * 
		 * @param p_allocator The new allocator function.
		 */
		void editAllocator(const Allocator& p_allocator)
		{
			_allocator = p_allocator;
		}

		/**
		 * @brief Updates the cleaner function used by the pool.
		 * 
		 * @param p_cleaner The new cleaner function.
		 */
		void editCleaner(const Cleaner& p_cleaner)
		{
			_cleaner = p_cleaner;
		}

		/**
		 * @brief Allocates a new object and adds it to the pool.
		 */
		void allocate()
		{
			_insert(_allocator());
		}

		/**
		 * @brief Retrieves the current size of the pool.
		 * 
		 * @return The number of preallocated objects available in the pool.
		 */
		size_t size() const
		{
			return (_preallocatedElements.size());
		}

		/**
		 * @brief Resizes the pool to the specified size.
		 * 
		 * @param p_newSize The new size of the pool.
		 */
		void resize(size_t p_newSize)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			while (_preallocatedElements.size() < p_newSize)
				_insert(_allocator());
		}

		/**
		 * @brief Obtains an object from the pool. Allocates a new object if the pool is empty.
		 * 
		 * The obtained object is cleaned using the cleaner function before being returned.
		 * 
		 * @return A managed object (unique_ptr) from the pool.
		 */
		Object obtain()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (_preallocatedElements.empty())
			{
				_insert(_allocator());
			}

			Object item(_preallocatedElements.front().release(), _destructorLambda);

			_preallocatedElements.pop_front();

			_cleaner(*item);
			return (std::move(item));
		}
	};
}
