#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace spk
{
	template <typename TKey, typename TValue>
	class ThreadSafeCollection
	{
	public:
		using key_type = TKey;
		using value_type = TValue;
		using pointer = std::shared_ptr<value_type>;

	private:
		mutable std::shared_mutex _mutex;
		std::unordered_map<key_type, pointer> _values;

	public:
		template <typename... TArgs>
		pointer emplace(
			const key_type &key,
			TArgs &&...args)
		{
			auto value =
				std::make_shared<value_type>(
					std::forward<TArgs>(args)...);

			std::unique_lock lock(_mutex);

			auto [it, inserted] =
				_values.emplace(key, value);

			if (!inserted)
			{
				throw std::logic_error(
					"Collection already contains the specified key");
			}

			return value;
		}

		void erase(const key_type &key)
		{
			std::unique_lock lock(_mutex);
			_values.erase(key);
		}

		bool contains(const key_type &key) const
		{
			std::shared_lock lock(_mutex);
			return _values.contains(key);
		}

		pointer get(const key_type &key) const
		{
			std::shared_lock lock(_mutex);
			return _values.at(key);
		}

		pointer tryGet(const key_type &key) const
		{
			std::shared_lock lock(_mutex);

			auto it = _values.find(key);

			if (it == _values.end())
			{
				return nullptr;
			}

			return it->second;
		}
	};
}