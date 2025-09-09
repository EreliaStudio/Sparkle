#pragma once

#include <functional>
#include <memory>

namespace spk
{
	template <typename TType>
	class CachedData
	{
	public:
		using Generator = std::function<TType()>;

	private:
		Generator _generator;
		mutable std::unique_ptr<TType> _data;

	public:
		CachedData(Generator p_generator) :
			_generator(p_generator)
		{
		}

		TType &get() const
		{
			if (_data == nullptr)
			{
				_data = std::make_unique<TType>(_generator());
			}
			return *_data;
		}

		TType &operator*() const
		{
			return get();
		}

		void release() const
		{
			_data.reset();
		}

		bool isCached() const
		{
			return (_data == nullptr);
		}
	};
}
