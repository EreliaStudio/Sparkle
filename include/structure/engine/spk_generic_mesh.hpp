#pragma once

#include "spk_sfinae.hpp"

#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace spk
{
	template <typename TVertexType>
	class GenericMesh
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

		unsigned int _getOrInsertVertex(const Vertex &p_v, std::unordered_map<Vertex, unsigned int> &p_vertexMap) const
		{
			auto insertResult = p_vertexMap.try_emplace(p_v, static_cast<unsigned int>(_buffer.vertices.size()));
			if (insertResult.second == true)
			{
				_buffer.vertices.push_back(p_v);
			}
			return insertResult.first->second;
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
		GenericMesh() = default;

		GenericMesh(const GenericMesh &p_other) :
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

		GenericMesh &operator=(const GenericMesh &p_other)
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

		GenericMesh(GenericMesh &&p_other) noexcept(std::is_nothrow_move_constructible_v<std::vector<Shape>> && std::is_nothrow_move_constructible_v<Buffer>) :
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

		GenericMesh &operator=(GenericMesh &&p_other) noexcept(std::is_nothrow_move_assignable_v<std::vector<Shape>> && std::is_nothrow_move_assignable_v<Buffer>)
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

		void addShape(std::vector<Vertex> &&p_vertices)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(Shape{std::move(p_vertices)});
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

			if (_needBake == false)
			{
				return;
			}

			// Clear and reserve to minimize reallocations during baking.
			_buffer.vertices.clear();
			_buffer.indexes.clear();

			std::size_t totalVertexCandidates = 0;
			std::size_t totalIndexCount = 0;
			for (const auto &s : _shapes)
			{
				totalVertexCandidates += s.points.size();
				if (s.points.size() >= 3)
				{
					totalIndexCount += (s.points.size() - 2) * 3;
				}
			}

			_buffer.vertices.reserve(totalVertexCandidates);
			_buffer.indexes.reserve(totalIndexCount);

			std::unordered_map<Vertex, unsigned int> vertexMap;
			vertexMap.reserve(totalVertexCandidates);

			for (const auto &s : _shapes)
			{
				_bakeShape(s, vertexMap);
			}

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
