#pragma once

#include <optional>
#include <functional>
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
			generator p_generator,
			destructor p_destructor = nullptr) :
			_generator(std::move(p_generator)),
			_destructor(std::move(p_destructor))
		{
		}

		~CachedData()
		{
			_destroyData();
		}

		CachedData(const CachedData &p_other)
			requires std::copy_constructible<value_type>
			:
			_generator(p_other._generator),
			_destructor(p_other._destructor)
		{
			if (p_other._data.has_value())
			{
				_data.emplace(*p_other._data);
			}
		}

		CachedData &operator=(const CachedData &p_other)
			requires std::copy_constructible<value_type>
		{
			if (this == &p_other)
			{
				return *this;
			}

			_destroyData();

			_generator = p_other._generator;
			_destructor = p_other._destructor;

			if (p_other._data.has_value())
			{
				_data.emplace(*p_other._data);
			}

			return *this;
		}

		CachedData(CachedData &&p_other) noexcept(std::is_nothrow_move_constructible_v<value_type>) :
			_generator(std::move(p_other._generator)),
			_destructor(std::move(p_other._destructor))
		{
			if (p_other._data.has_value())
			{
				_data.emplace(std::move(*p_other._data));
				p_other._data.reset();
			}
		}

		CachedData &operator=(CachedData &&p_other) noexcept(
			std::is_nothrow_move_constructible_v<value_type> &&
			std::is_nothrow_destructible_v<value_type>)
		{
			if (this == &p_other)
			{
				return *this;
			}

			_destroyData();

			_generator = std::move(p_other._generator);
			_destructor = std::move(p_other._destructor);

			if (p_other._data.has_value())
			{
				_data.emplace(std::move(*p_other._data));
				p_other._data.reset();
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
		void set(TValue &&p_value)
		{
			_destroyData();
			_data.emplace(std::forward<TValue>(p_value));
		}

		template <typename... TArguments>
			requires std::constructible_from<value_type, TArguments &&...>
		value_type &emplace(TArguments &&...p_arguments)
		{
			_destroyData();

			return _data.emplace(std::forward<TArguments>(p_arguments)...);
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