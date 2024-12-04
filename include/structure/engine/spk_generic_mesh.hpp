#pragma once

#include "spk_sfinae.hpp"
#include "structure/design_pattern/spk_cached_data.hpp"

#include <initializer_list>
#include <span>
#include <type_traits>
#include <vector>

namespace spk
{
	template <typename TVertexType>
	class GenericMesh
	{
	public:
		using Vertex = TVertexType;
		using Shape = std::span<Vertex>;
		using ConstShape = std::span<const Vertex>;

		struct Buffer
		{
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indexes;

			void clear()
			{
				vertices.clear();
				indexes.clear();
			}
		};

	private:
		Buffer _buffer;

		struct ShapeIdentifier
		{
			std::size_t startIndex;
			std::size_t endIndex;
		};
		std::vector<ShapeIdentifier> _shapeIdentifiers;

		std::vector<Shape> _generateMutableShapes()
		{
			std::vector<Shape> out;
			out.reserve(_shapeIdentifiers.size());

			Vertex *base = _buffer.vertices.data();
			for (const auto &id : _shapeIdentifiers)
			{
				const std::size_t count = id.endIndex - id.startIndex;
				out.emplace_back(base + id.startIndex, count);
			}
			return out;
		}

		std::vector<ConstShape> _generateConstShapes() const
		{
			std::vector<ConstShape> out;
			out.reserve(_shapeIdentifiers.size());

			const Vertex *base = _buffer.vertices.data();
			for (const auto &id : _shapeIdentifiers)
			{
				const std::size_t count = id.endIndex - id.startIndex;
				out.emplace_back(base + id.startIndex, count);
			}
			return out;
		}

		CachedData<std::vector<Shape>> _shapesCache;
		CachedData<std::vector<ConstShape>> _constShapesCache;

		void _invalidateCaches()
		{
			_shapesCache.release();
			_constShapesCache.release();
		}

	public:
		GenericMesh() :
			_shapesCache([this] { return _generateMutableShapes(); }),
			_constShapesCache([this] { return _generateConstShapes(); })
		{
		}

		GenericMesh(const GenericMesh &other) :
			_buffer(other._buffer),
			_shapeIdentifiers(other._shapeIdentifiers),
			_shapesCache([this] { return _generateMutableShapes(); }),
			_constShapesCache([this] { return _generateConstShapes(); })
		{
		}

		GenericMesh &operator=(const GenericMesh &other)
		{
			if (this == &other)
			{
				return *this;
			}
			_buffer = other._buffer;
			_shapeIdentifiers = other._shapeIdentifiers;
			_invalidateCaches();
			return *this;
		}

		GenericMesh(GenericMesh &&other) noexcept :
			_buffer(std::move(other._buffer)),
			_shapeIdentifiers(std::move(other._shapeIdentifiers)),
			_shapesCache([this] { return _generateMutableShapes(); }),
			_constShapesCache([this] { return _generateConstShapes(); })
		{
			other._buffer.clear();
			other._shapeIdentifiers.clear();
		}

		GenericMesh &operator=(GenericMesh &&other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			_buffer = std::move(other._buffer);
			_shapeIdentifiers = std::move(other._shapeIdentifiers);
			_invalidateCaches();
			other._buffer.clear();
			other._shapeIdentifiers.clear();
			return *this;
		}

		void clear()
		{
			_shapeIdentifiers.clear();
			_buffer.clear();
			_invalidateCaches();
		}

		void addShape(const Vertex &a, const Vertex &b, const Vertex &c)
		{
			const std::size_t base = _buffer.vertices.size();
			_buffer.vertices.reserve(base + 3);
			_buffer.indexes.reserve(_buffer.indexes.size() + 3);

			_buffer.vertices.push_back(a);
			_buffer.vertices.push_back(b);
			_buffer.vertices.push_back(c);

			_buffer.indexes.push_back(static_cast<unsigned int>(base + 0));
			_buffer.indexes.push_back(static_cast<unsigned int>(base + 1));
			_buffer.indexes.push_back(static_cast<unsigned int>(base + 2));

			_shapeIdentifiers.push_back({base, base + 3});
			_invalidateCaches();
		}

		void addShape(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d)
		{
			const std::size_t base = _buffer.vertices.size();
			_buffer.vertices.reserve(base + 4);
			_buffer.indexes.reserve(_buffer.indexes.size() + 6);

			_buffer.vertices.push_back(a);
			_buffer.vertices.push_back(b);
			_buffer.vertices.push_back(c);
			_buffer.vertices.push_back(d);

			const auto baseU = static_cast<unsigned int>(base);
			_buffer.indexes.push_back(baseU + 0);
			_buffer.indexes.push_back(baseU + 1);
			_buffer.indexes.push_back(baseU + 2);
			_buffer.indexes.push_back(baseU + 0);
			_buffer.indexes.push_back(baseU + 2);
			_buffer.indexes.push_back(baseU + 3);

			_shapeIdentifiers.push_back({base, base + 4});
			_invalidateCaches();
		}

		void addShape(std::span<const Vertex> points)
		{
			if (points.size() < 3)
			{
				return;
			}

			const std::size_t base = _buffer.vertices.size();
			_buffer.vertices.reserve(base + points.size());
			_buffer.vertices.insert(_buffer.vertices.end(), points.begin(), points.end());

			const auto triCount = points.size() - 2;
			_buffer.indexes.reserve(_buffer.indexes.size() + triCount * 3);

			const auto baseU = static_cast<unsigned int>(base);
			for (std::size_t i = 1; i + 1 < points.size(); ++i)
			{
				_buffer.indexes.push_back(baseU + 0);
				_buffer.indexes.push_back(baseU + static_cast<unsigned int>(i));
				_buffer.indexes.push_back(baseU + static_cast<unsigned int>(i + 1));
			}

			_shapeIdentifiers.push_back({base, base + points.size()});
			_invalidateCaches();
		}

		void addShape(const std::vector<Vertex> &container)
		{
			addShape(std::span<const Vertex>(container.data(), container.size()));
		}

		std::vector<Shape> &shapes()
		{
			return _shapesCache.get();
		}

		const std::vector<ConstShape> &shapes() const
		{
			return _constShapesCache.get();
		}

		Buffer &buffer()
		{
			return _buffer;
		}
		const Buffer &buffer() const
		{
			return _buffer;
		}
	};
}
