#pragma once

#include <functional>
#include <optional>
#include <stdexcept>

namespace spk
{
	template <typename TType>
	class CachedData
	{
	public:
		using value_type = TType;
		using generator = std::function<TType()>;
		using destructor = std::function<void(TType &)>;

	private:
		generator _generator = nullptr;
		destructor _destructor = nullptr;
		mutable std::optional<value_type> _data;

		void _generateData() const
		{
			if (_data.has_value())
			{
				return;
			}

			if (_generator == nullptr)
			{
				throw std::runtime_error("CachedData: generator not set");
			}

			_data.emplace(_generator());
		}

		void _destroyData() const
		{
			if (_data.has_value() == false)
			{
				return;
			}

			if (_destructor != nullptr)
			{
				_destructor(*_data);
			}

			_data.reset();
		}

	public:
		CachedData() = default;

		explicit CachedData(
			generator generator,
			destructor destructor = nullptr) :
			_generator(std::move(generator)),
			_destructor(std::move(destructor))
		{
		}

		~CachedData()
		{
			_destroyData();
		}

		CachedData(const CachedData &other)
			requires std::copy_constructible<value_type>
			:
			_generator(other._generator),
			_destructor(other._destructor)
		{
			if (other._data.has_value())
			{
				_data.emplace(*other._data);
			}
		}

		CachedData &operator=(const CachedData &other)
			requires std::copy_constructible<value_type>
		{
			if (this == &other)
			{
				return *this;
			}

			_destroyData();

			_generator = other._generator;
			_destructor = other._destructor;

			if (other._data.has_value())
			{
				_data.emplace(*other._data);
			}

			return *this;
		}

		CachedData(CachedData &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>) :
			_generator(std::move(other._generator)),
			_destructor(std::move(other._destructor))
		{
			if (other._data.has_value())
			{
				_data.emplace(std::move(*other._data));
				other._data.reset();
			}
		}

		CachedData &operator=(CachedData &&other) noexcept(
			std::is_nothrow_move_constructible_v<value_type> &&
			std::is_nothrow_destructible_v<value_type>)
		{
			if (this == &other)
			{
				return *this;
			}

			_destroyData();

			_generator = std::move(other._generator);
			_destructor = std::move(other._destructor);

			if (other._data.has_value())
			{
				_data.emplace(std::move(*other._data));
				other._data.reset();
			}

			return *this;
		}

		[[nodiscard]] value_type &get()
		{
			_generateData();
			return *_data;
		}

		[[nodiscard]] const value_type &get() const
		{
			_generateData();
			return *_data;
		}

		[[nodiscard]] operator const value_type &() const
		{
			return get();
		}

		[[nodiscard]] value_type &operator*()
		{
			return get();
		}

		[[nodiscard]] const value_type &operator*() const
		{
			return get();
		}

		[[nodiscard]] value_type *operator->()
		{
			return &get();
		}

		[[nodiscard]] const value_type *operator->() const
		{
			return &get();
		}

		void invalidate() const
		{
			_destroyData();
		}

		value_type &refresh()
		{
			invalidate();
			return get();
		}

		const value_type &refresh() const
		{
			invalidate();
			return get();
		}

		template <typename TValue>
			requires std::constructible_from<value_type, TValue &&>
		void set(TValue &&value)
		{
			_destroyData();
			_data.emplace(std::forward<TValue>(value));
		}

		template <typename... TArguments>
			requires std::constructible_from<value_type, TArguments &&...>
		value_type &emplace(TArguments &&...arguments)
		{
			_destroyData();

			return _data.emplace(std::forward<TArguments>(arguments)...);
		}

		[[nodiscard]] std::optional<value_type> take()
			requires std::move_constructible<value_type>
		{
			if (_data.has_value() == false)
			{
				return std::nullopt;
			}

			std::optional<value_type> result(
				std::move(*_data));

			_data.reset();

			return result;
		}
	};
}
