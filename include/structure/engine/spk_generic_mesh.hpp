#pragma once

#include "spk_sfinae.hpp"

#include <cstring> // memcpy
#include <initializer_list>
#include <mutex>
#include <type_traits>
#include <vector>

namespace spk
{
	template <typename TVertexType>
	class GenericMesh
	{
	public:
		using Vertex = TVertexType;

		using Shape = std::vector<Vertex>;

		struct Buffer
		{
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indexes;
		};

	private:
		std::vector<Shape> _shapes;

		mutable bool _needBake = false;
		mutable std::size_t _bakedShapeCount = 0;

		mutable Buffer _buffer;
		mutable std::mutex _mutex;

		void _appendShapeFast(const Shape &s) const
		{
			const std::size_t n = s.size();
			if (n < 3)
			{
				return;
			}

			const unsigned int base = static_cast<unsigned int>(_buffer.vertices.size());

			const std::size_t oldV = _buffer.vertices.size();
			_buffer.vertices.resize(oldV + n);
			if constexpr (std::is_trivially_copyable_v<Vertex>)
			{
				std::memcpy(&_buffer.vertices[oldV], s.data(), n * sizeof(Vertex));
			}
			else
			{
				std::copy(s.begin(), s.end(), _buffer.vertices.begin() + static_cast<std::ptrdiff_t>(oldV));
			}

			const std::size_t triCount = n - 2;
			const std::size_t oldI = _buffer.indexes.size();
			_buffer.indexes.resize(oldI + triCount * 3);

			unsigned int *dst = _buffer.indexes.data() + oldI;
			for (unsigned int i = 1; i + 1 < n; ++i)
			{
				*dst++ = base;
				*dst++ = base + i;
				*dst++ = base + i + 1;
			}
		}

		void _shrinkOneBakedShape() const
		{
			if (_bakedShapeCount == 0)
			{
				return;
			}
			const auto &s = _shapes[_bakedShapeCount - 1];
			const std::size_t n = s.size();

			if (n > 0 && _buffer.vertices.size() >= n)
			{
				_buffer.vertices.resize(_buffer.vertices.size() - n);
			}
			if (n >= 3)
			{
				const std::size_t k = (n - 2) * 3;
				if (_buffer.indexes.size() >= k)
				{
					_buffer.indexes.resize(_buffer.indexes.size() - k);
				}
			}
			--_bakedShapeCount;
		}

	public:
		GenericMesh() = default;

		GenericMesh(const GenericMesh &p_other) :
			_shapes(),
			_needBake(false),
			_bakedShapeCount(0),
			_buffer(),
			_mutex()
		{
			std::scoped_lock lock(p_other._mutex);
			_shapes = p_other._shapes;
			_needBake = p_other._needBake;
			_bakedShapeCount = p_other._bakedShapeCount;
			_buffer = p_other._buffer;
		}

		GenericMesh &operator=(const GenericMesh &p_other)
		{
			if (this == &p_other)
			{
				return *this;
			}
			std::scoped_lock lock(_mutex, p_other._mutex);
			_shapes = p_other._shapes;
			_needBake = p_other._needBake;
			_bakedShapeCount = p_other._bakedShapeCount;
			_buffer = p_other._buffer;
			return *this;
		}

		GenericMesh(GenericMesh &&p_other) :
			_shapes(),
			_needBake(false),
			_bakedShapeCount(0),
			_buffer(),
			_mutex()
		{
			std::scoped_lock lock(p_other._mutex);
			_shapes = std::move(p_other._shapes);
			_needBake = p_other._needBake;
			_bakedShapeCount = p_other._bakedShapeCount;
			_buffer = std::move(p_other._buffer);

			p_other._needBake = false;
			p_other._bakedShapeCount = 0;
			p_other._buffer.vertices.clear();
			p_other._buffer.indexes.clear();
			p_other._shapes.clear();
		}

		GenericMesh &operator=(GenericMesh &&p_other)
		{
			if (this == &p_other)
			{
				return *this;
			}
			std::scoped_lock lock(_mutex, p_other._mutex);

			_shapes = std::move(p_other._shapes);
			_needBake = p_other._needBake;
			_bakedShapeCount = p_other._bakedShapeCount;
			_buffer = std::move(p_other._buffer);

			p_other._needBake = false;
			p_other._bakedShapeCount = 0;
			p_other._buffer.vertices.clear();
			p_other._buffer.indexes.clear();
			p_other._shapes.clear();

			return *this;
		}

		void resize(const size_t& p_size)
		{
			_shapes.resize(p_size);
		}

		void addShape(const Shape &p_shapes)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(p_shapes);
			_needBake = true;
		}

		void addShape(Shape &&p_shapes)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(std::move(p_shapes));
			_needBake = true;
		}

		void addShape(std::initializer_list<Vertex> p_vertices)
		{
			addShape(std::vector<Vertex>(p_vertices));
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c)
		{
			addShape(std::vector<Vertex>{p_a, p_b, p_c});
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c, const Vertex &p_d)
		{
			addShape(std::vector<Vertex>{p_a, p_b, p_c, p_d});
		}

		void popShape()
		{
			std::lock_guard lock(_mutex);
			if (_shapes.empty() == false)
			{
				_shapes.pop_back();
				_needBake = true;
			}
		}

		void clear()
		{
			std::lock_guard lock(_mutex);
			_shapes.clear();
			_buffer.vertices.clear();
			_buffer.indexes.clear();
			_bakedShapeCount = 0;
			_needBake = false;
		}

		const std::vector<Shape> &shapes() const
		{
			return _shapes;
		}

		std::vector<Shape> &shapes()
		{
			std::lock_guard lock(_mutex);
			_needBake = true;
			_bakedShapeCount = 0;
			return _shapes;
		}

		auto begin() const
		{
			return _shapes.begin();
		}
		auto end() const
		{
			return _shapes.end();
		}

		bool hasPendingChanges() const
		{
			std::lock_guard lock(_mutex);
			return _needBake;
		}

		void unbake()
		{
			std::lock_guard lock(_mutex);
			_needBake = true;
			_bakedShapeCount = 0;

			_buffer.vertices.clear();
			_buffer.indexes.clear();
		}

		void bake() const
		{
			std::lock_guard lock(_mutex);
			if (_needBake == false)
			{
				return;
			}

			while (_bakedShapeCount > _shapes.size())
			{
				_shrinkOneBakedShape();
			}

			if (_bakedShapeCount == 0)
			{
				std::size_t totalV = 0, totalI = 0;
				for (const auto &s : _shapes)
				{
					const std::size_t n = s.size();
					totalV += n;
					if (n >= 3)
					{
						totalI += (n - 2) * 3;
					}
				}
				_buffer.vertices.clear();
				_buffer.indexes.clear();
				_buffer.vertices.reserve(totalV);
				_buffer.indexes.reserve(totalI);
			}
			else
			{
				std::size_t addV = 0, addI = 0;
				for (std::size_t i = _bakedShapeCount; i < _shapes.size(); ++i)
				{
					const std::size_t n = _shapes[i].size();
					addV += n;
					if (n >= 3)
					{
						addI += (n - 2) * 3;
					}
				}
				_buffer.vertices.reserve(_buffer.vertices.size() + addV);
				_buffer.indexes.reserve(_buffer.indexes.size() + addI);
			}

			for (std::size_t i = _bakedShapeCount; i < _shapes.size(); ++i)
			{
				_appendShapeFast(_shapes[i]);
			}
			_bakedShapeCount = _shapes.size();
			_needBake = false;
		}

		const Buffer &buffer() const
		{
			if (_needBake == true)
			{
				bake();
			}
			return _buffer;
		}

		void reserveShapes(size_t p_count)
		{
			std::lock_guard lock(_mutex);
			_shapes.reserve(p_count);
		}
	};
}
