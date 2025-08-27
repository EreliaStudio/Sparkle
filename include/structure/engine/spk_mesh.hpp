#pragma once

#include "spk_sfinae.hpp"
#include "structure/engine/spk_vertex.hpp"

#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace spk
{
	template <typename TVertexType>
	class TMesh
	{
	public:
		using Vertex = TVertexType;

		struct Shape
		{
			std::vector<Vertex> points;
		};

		struct Buffer
		{
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indexes;
		};

	private:
		std::vector<Shape> _shapes;

		mutable bool _needBake = false;
		mutable Buffer _buffer;

		mutable std::mutex _mutex;

		unsigned int _getOrInsertVertex(const Vertex &v, std::unordered_map<Vertex, unsigned int> &vertexMap) const
		{
			auto it = vertexMap.find(v);
			if (it != vertexMap.end())
			{
				return it->second;
			}
			else
			{
				const unsigned int newIndex = static_cast<unsigned int>(_buffer.vertices.size());
				_buffer.vertices.push_back(v);
				vertexMap[v] = newIndex;
				return newIndex;
			}
		}

		void _bakeShape(const Shape &p_shape, std::unordered_map<Vertex, unsigned int> &p_vertexMap) const
		{
			if (p_shape.points.size() < 3)
			{
				return;
			}
			unsigned int i0 = _getOrInsertVertex(p_shape.points[0], p_vertexMap);
			for (size_t i = 1; i + 1 < p_shape.points.size(); ++i)
			{
				unsigned int i1 = _getOrInsertVertex(p_shape.points[i], p_vertexMap);
				unsigned int i2 = _getOrInsertVertex(p_shape.points[i + 1], p_vertexMap);
				_buffer.indexes.push_back(i0);
				_buffer.indexes.push_back(i1);
				_buffer.indexes.push_back(i2);
			}
		}

	public:
		TMesh() = default;

		TMesh(const TMesh &p_other) :
			_shapes(),
			_needBake(false),
			_buffer(),
			_mutex()
		{
			std::scoped_lock lock(p_other._mutex);
			_shapes = p_other._shapes;
			_needBake = p_other._needBake;
			_buffer = p_other._buffer;
		}

		TMesh &operator=(const TMesh &p_other)
		{
			if (this == &p_other)
			{
				return *this;
			}

			std::scoped_lock lock(_mutex, p_other._mutex);
			_shapes = p_other._shapes;
			_needBake = p_other._needBake;
			_buffer = p_other._buffer;
			return *this;
		}

		TMesh(TMesh &&p_other) noexcept(std::is_nothrow_move_constructible_v<std::vector<Shape>> && std::is_nothrow_move_constructible_v<Buffer>) :
			_shapes(),
			_needBake(false),
			_buffer(),
			_mutex()
		{
			std::scoped_lock lock(p_other._mutex);
			_shapes = std::move(p_other._shapes);
			_needBake = p_other._needBake;
			_buffer = std::move(p_other._buffer);

			p_other._needBake = false;
			p_other._buffer.vertices.clear();
			p_other._buffer.indexes.clear();
			p_other._shapes.clear();
		}

		TMesh &operator=(TMesh &&p_other) noexcept(std::is_nothrow_move_assignable_v<std::vector<Shape>> && std::is_nothrow_move_assignable_v<Buffer>)
		{
			if (this == &p_other)
			{
				return *this;
			}
			std::scoped_lock lock(_mutex, p_other._mutex);

			_shapes = std::move(p_other._shapes);
			_needBake = p_other._needBake;
			_buffer = std::move(p_other._buffer);

			p_other._needBake = false;
			p_other._buffer.vertices.clear();
			p_other._buffer.indexes.clear();
			p_other._shapes.clear();

			return *this;
		}

		void addShape(const std::vector<Vertex> &p_vertices)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(Shape{p_vertices});
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
			if (!_shapes.empty())
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
			_needBake = false;
		}

		const std::vector<Shape> &shapes() const
		{
			return _shapes;
		}

		std::vector<Shape> &shapes()
		{
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
			_buffer.vertices.clear();
			_buffer.indexes.clear();
		}

		void bake() const
		{
			std::lock_guard lock(_mutex);

			_buffer.vertices.clear();
			_buffer.indexes.clear();

			std::unordered_map<Vertex, unsigned int> vertexMap;

			for (const auto &s : _shapes)
			{
				_bakeShape(s, vertexMap);
			}

			_needBake = false;
		}

		const Buffer &buffer() const
		{
			if (_needBake)
			{
				bake();
			}
			return _buffer;
		}
	};
}