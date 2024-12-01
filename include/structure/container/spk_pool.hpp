#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <mutex>

namespace spk
{
	template<typename TType>
	class Pool
	{
	public:
		using Allocator = typename std::function<TType* (void)>;

		using Cleaner = typename std::function<void(TType&)>;

	private:
		using Destructor = typename std::function<void(TType* p_toReturn)>;

	public:
		using Object = typename std::unique_ptr<TType, Destructor>;

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
		Pool() :
			_allocator([]() {return (new TType()); }),
			_cleaner([](TType& toClean) {})
		{

		}

		Pool(const Allocator& p_allocator, const Cleaner& p_cleaner) :
			_allocator(p_allocator),
			_cleaner(p_cleaner)
		{

		}

		void editAllocator(const Allocator& p_allocator)
		{
			_allocator = p_allocator;
		}

		void editCleaner(const Cleaner& p_cleaner)
		{
			_cleaner = p_cleaner;
		}

		void allocate()
		{
			_insert(_allocator());
		}

		size_t size() const
		{
			return (_preallocatedElements.size());
		}

		void resize(size_t p_newSize)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			while (_preallocatedElements.size() < p_newSize)
				_insert(_allocator());
		}

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
