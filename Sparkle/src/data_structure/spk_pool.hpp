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
	public:
		/**
		 * @brief Type alias for the allocator function.
		 * 
		 * Defines an allocator function type that is used to create new instances of TType.
		 * This function allows for custom allocation logic to be provided when constructing
		 * objects managed by the pool. It should return a pointer to a newly created instance
		 * of TType.
		 */
		using Allocator = typename std::function<TType*(void)>;

		/**
		 * @brief Type alias for the cleaner function.
		 * 
		 * Defines an cleaner function type that is used when the user require an element of the pool.
		 * This function allows for custom reset logic to be provided when returning an
		 * objects managed by the pool. It should return nothing and should get a reference to
		 * the object to "clean".
		 */
		using Cleaner = typename std::function<void(TType&)>;

	private:	
		using Destructor = typename std::function<void(TType* p_toReturn)>;
	
	public:
		/**
		 * @brief Describe the type of the provided object given by the pool uppon request
		*/
		using Object = typename std::unique_ptr<TType, Destructor>;

	private:
		std::recursive_mutex _mutex;
		Allocator _allocator;
		Cleaner _cleaner;
		std::deque<std::unique_ptr<TType>> _preallocatedElements;

		const Destructor _destructorLambda = [&](TType* p_toReturn){ _insert(p_toReturn); };

		void _insert(TType* p_toReturn)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_preallocatedElements.push_back(std::unique_ptr<TType>(p_toReturn));
		}

	public:
		Pool() :
			_allocator([](){return (new TType());}),
			_cleaner([](TType& toClean){})
		{
			
		}

		/**
		 * @brief Constructor
		 * 
		 * Initializes a new instance of the Pool class with a specified allocator.
		 * 
		 * @param p_allocator The initial allocator of the pool.
		 * @param p_cleaner The initial cleaner of the pool. Called before returning an element from the pool, to "reset" it to value specified by user.
		 */
		Pool(const Allocator& p_allocator, const Cleaner& p_cleaner) :
			_allocator(p_allocator),
			_cleaner(p_cleaner)
		{
			
		}

		/**
		 * @brief Edits the allocator used by the pool.
		 * 
		 * Allows for the modification of the allocator function after the pool has been
		 * created. This can be useful for changing the logic used to create new instances
		 * of TType, for example, to use a different constructor or to recycle objects in a
		 * specific way.
		 * 
		 * @param p_allocator The new allocator function to use for object creation.
		 */
		void editAllocator(const Allocator& p_allocator)
		{   
			_allocator = p_allocator;
		}

		/**
		 * @brief Edits the cleaner used by the pool.
		 * 
		 * Allows for the modification of the cleaner function after the pool has been
		 * created. This can be useful for changing the logic used to provide a "fresh" object
		 * to the user.
		 * 
		 * @param p_cleaner The new cleaner function to before giving an object to user.
		 */
		void editCleaner(const Cleaner& p_cleaner)
		{   
			_cleaner = p_cleaner;
		}

		/**
		 * @brief Allocates a new object and adds it to the pool.
		 * 
		 * Uses the current allocator to create a new instance of TType, which is then added
		 * to the pool. This method can be used to manually increase the pool's size or to
		 * prepopulate the pool with a set of objects before they are needed.
		 */
		void allocate()
		{
			_insert(_allocator());
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

			while (_preallocatedElements.size() < p_newSize)
				_insert(_allocator());
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
				_insert(_allocator());
			}

			Object item(_preallocatedElements.front().release(), _destructorLambda);

			_preallocatedElements.pop_front();

			_cleaner(*item);
			return (std::move(item));
		}
	};
}
