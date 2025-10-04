#pragma once

namespace spk
{
	template <typename T>
	class Grid2D
	{
	public:
		using Unit = T;

		struct Row
		{
			Unit *ptr;
			size_t len;
			Unit &operator[](size_t y)
			{
				return ptr[y];
			}
			const Unit &operator[](size_t y) const
			{
				return ptr[y];
			}
		};

		struct ConstRow
		{
			const Unit *ptr;
			size_t len;
			const Unit &operator[](size_t y) const
			{
				return ptr[y];
			}
		};

	private:
		spk::Vector2UInt _size{};
		std::vector<Unit> _data;

		static size_t _index(size_t x, size_t y, size_t height)
		{
			return x * height + y;
		}

	public:
		Grid2D() = default;

		explicit Grid2D(const spk::Vector2UInt &size, const Unit &fill = Unit{}) :
			_size(size),
			_data(static_cast<size_t>(size.x) * size.y, fill)
		{
		}

		const spk::Vector2UInt &size() const
		{
			return _size;
		}
		size_t width() const
		{
			return _size.x;
		}
		size_t height() const
		{
			return _size.y;
		}

		Unit &operator()(size_t x, size_t y)
		{
			return _data[_index(x, y, _size.y)];
		}
		const Unit &operator()(size_t x, size_t y) const
		{
			return _data[_index(x, y, _size.y)];
		}

		Row operator[](size_t x)
		{
			return Row{_data.data() + x * _size.y, _size.y};
		}
		ConstRow operator[](size_t x) const
		{
			return ConstRow{_data.data() + x * _size.y, _size.y};
		}

		Unit *data()
		{
			return _data.data();
		}
		const Unit *data() const
		{
			return _data.data();
		}
	};
}