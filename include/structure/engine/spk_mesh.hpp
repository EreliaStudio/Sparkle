#pragma once

#include "spk_sfinae.hpp"
#include "structure/engine/spk_vertex.hpp"

#include <filesystem>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace spk
{
	template <typename TVertexType>
	class TMesh
	{
	public:
		using Vertex = TVertexType;

		struct Triangle
		{
			Vertex a;
			Vertex b;
			Vertex c;
		};

		struct Quad
		{
			Vertex a;
			Vertex b;
			Vertex c;
			Vertex d;
		};

		struct Buffer
		{
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indexes;
		};

	private:
		using Shape = std::variant<Triangle, Quad>;

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

		void _bakeTriangle(const Triangle &t, std::unordered_map<Vertex, unsigned int> &vertexMap) const
		{
			_buffer.indexes.push_back(_getOrInsertVertex(t.a, vertexMap));
			_buffer.indexes.push_back(_getOrInsertVertex(t.b, vertexMap));
			_buffer.indexes.push_back(_getOrInsertVertex(t.c, vertexMap));
		}

		void _bakeQuad(const Quad &q, std::unordered_map<Vertex, unsigned int> &vertexMap) const
		{
			unsigned int ia = _getOrInsertVertex(q.a, vertexMap);
			unsigned int ib = _getOrInsertVertex(q.b, vertexMap);
			unsigned int ic = _getOrInsertVertex(q.c, vertexMap);
			unsigned int id = _getOrInsertVertex(q.d, vertexMap);

			_buffer.indexes.push_back(ia);
			_buffer.indexes.push_back(ib);
			_buffer.indexes.push_back(ic);

			_buffer.indexes.push_back(ia);
			_buffer.indexes.push_back(ic);
			_buffer.indexes.push_back(id);
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

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(Triangle{p_a, p_b, p_c});
			_needBake = true;
		}

		void addShape(const Triangle &p_triangle)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(p_triangle);
			_needBake = true;
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c, const Vertex &p_d)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(Quad{p_a, p_b, p_c, p_d});
			_needBake = true;
		}

		void addShape(const Quad &p_quad)
		{
			std::lock_guard lock(_mutex);
			_shapes.push_back(p_quad);
			_needBake = true;
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
				if (std::holds_alternative<Triangle>(s))
				{
					_bakeTriangle(std::get<Triangle>(s), vertexMap);
				}
				else
				{
					_bakeQuad(std::get<Quad>(s), vertexMap);
				}
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