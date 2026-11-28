#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>

#include "container/buffer.hpp"
#include "geometry/mesh_data.hpp"

namespace spk
{
	template <typename TVertexType>
	class Mesh
	{
	public:
		using Vertex = TVertexType;
		using Index = std::uint32_t;

	private:
		std::shared_ptr<const MeshData> _content;

	protected:
		void _attachContent(std::shared_ptr<const MeshData> content)
		{
			_content = std::move(content);
		}

	public:
		template <typename TMeshType>
		class Builder
		{
		private:
			VertexLayout _layout;
			bool _consumed = false;
			Buffer::Builder _vertices;
			Buffer::Builder _indices;
			bool _attributesInitialized = false;

			void _check() const
			{
				if (_consumed)
				{
					throw std::logic_error("Mesh::Builder was already consumed");
				}
			}

			void _initializeAttributes()
			{
				if (_attributesInitialized)
				{
					return;
				}

				_setupAttributes(_layout);
				if (_layout.stride() != sizeof(Vertex))
				{
					throw std::logic_error("Mesh vertex type does not match the layout stride");
				}
				_attributesInitialized = true;
			}

		protected:
			virtual void _setupAttributes(VertexLayout &) = 0;
			[[nodiscard]] virtual std::size_t _vertexCount() const
			{
				return _vertices.size() / sizeof(Vertex);
			}
			virtual void _setupMesh(TMeshType &mesh)
			{
			}

		public:
			Builder() = default;

			virtual ~Builder() = default;

			[[nodiscard]] Index addVertex(const Vertex &vertex)
			{
				_check();
				_initializeAttributes();

				const std::size_t vertexCount = _vertexCount();

				if (vertexCount > std::numeric_limits<Index>::max())
				{
					throw std::overflow_error("Mesh vertex index overflow");
				}

				_vertices.pushBack(vertex);

				return static_cast<Index>(vertexCount);
			}

			// Batch writes avoid per-element buffer resizing.
			[[nodiscard]] Index addVertices(std::span<const Vertex> vertices)
			{
				_check();
				_initializeAttributes();

				const std::size_t first = _vertexCount();
				if (first > std::numeric_limits<Index>::max() ||
					vertices.size() > std::numeric_limits<Index>::max() - first)
				{
					throw std::overflow_error("Mesh vertex index overflow");
				}
				_vertices.insert(vertices);
				return static_cast<Index>(first);
			}

			// Populate the final vertex buffer directly when the source requires a conversion.
			template <typename TGenerator>
			[[nodiscard]] Index addGeneratedVertices(std::size_t count, TGenerator &&generator)
			{
				_check();
				_initializeAttributes();

				const std::size_t first = _vertexCount();
				if (first > std::numeric_limits<Index>::max() ||
					count > std::numeric_limits<Index>::max() - first)
				{
					throw std::overflow_error("Mesh vertex index overflow");
				}

				auto &buffer = _vertices;
				buffer.template resize<Vertex>(first + count);
				auto output = buffer.template cast<Vertex>().subspan(first, count);
				for (std::size_t index = 0; index < count; ++index)
				{
					output[index] = std::invoke(generator, index);
				}
				return static_cast<Index>(first);
			}

			void addIndices(std::span<const Index> indices)
			{
				_check();
				const auto count = _vertices.size() / sizeof(Vertex);
				if (std::any_of(indices.begin(), indices.end(), [count](Index index) {
						return index >= count;
					}))
				{
					throw std::out_of_range("Mesh index references a missing vertex");
				}
				_indices.insert(indices);
			}

			// Avoid an intermediate allocation when each vertex is already one triangle-list index.
			void addSequentialIndices(std::size_t count)
			{
				_check();
				if (count > _vertexCount() || count > std::numeric_limits<Index>::max())
				{
					throw std::out_of_range("Mesh sequential index references a missing vertex");
				}

				auto &buffer = _indices;
				const std::size_t previousCount = buffer.size() / sizeof(Index);
				if (count > std::numeric_limits<std::size_t>::max() - previousCount)
				{
					throw std::overflow_error("Mesh index count overflow");
				}
				buffer.template resize<Index>(previousCount + count);
				auto output = buffer.template cast<Index>().subspan(previousCount, count);
				std::iota(output.begin(), output.end(), Index{0});
			}

			void addIndex(Index index)
			{
				_check();

				if (index >= _vertices.size() / sizeof(Vertex))
				{
					throw std::out_of_range("Mesh index references a missing vertex");
				}

				_indices.pushBack(index);
			}

			void addTriangle(Index a, Index b, Index c)
			{
				addIndex(a);
				addIndex(b);
				addIndex(c);
			}

			void addShape(
				const Vertex &topLeft,
				const Vertex &bottomLeft,
				const Vertex &bottomRight,
				const Vertex &topRight)
			{
				const Index first = addVertex(topLeft);

				(void)addVertex(bottomLeft);
				(void)addVertex(bottomRight);
				(void)addVertex(topRight);

				addTriangle(first, first + 1, first + 2);
				addTriangle(first, first + 2, first + 3);
			}

			void reserve(std::size_t vertexCount, std::size_t indexCount)
			{
				_check();
				_initializeAttributes();

				_vertices.template reserve<Vertex>(vertexCount);
				_indices.template reserve<Index>(indexCount);
			}

			template <typename... TArgs>
				requires std::constructible_from<TMeshType, TArgs...>
			[[nodiscard]] TMeshType build(TArgs &&...args) &&
			{
				_check();
				_initializeAttributes();

				auto vertices = std::move(_vertices).build();
				auto indices = std::move(_indices).build();
				_consumed = true;

				TMeshType result(std::forward<TArgs>(args)...);
				result._attachContent(std::make_shared<const MeshData>(std::move(vertices), std::move(indices), std::move(_layout)));

				_setupMesh(result);

				return result;
			}
		};

		Mesh() = default;

		[[nodiscard]] const MeshData &content() const
		{
			if (_content == nullptr)
			{
				throw std::logic_error("Mesh is empty");
			}
			return *_content;
		}

		[[nodiscard]] std::uint64_t identifier() const noexcept
		{
			return _content ? _content->identifier() : 0;
		}

		[[nodiscard]] const Buffer::Immutable &vertexBuffer() const
		{
			return content().vertexBuffer();
		}

		[[nodiscard]] const Buffer::Immutable &indexBuffer() const
		{
			return content().indexBuffer();
		}

		[[nodiscard]] const VertexLayout &vertexLayout() const
		{
			return content().vertexLayout();
		}

		[[nodiscard]] std::span<const Vertex> vertices() const
		{
			return _content ? vertexBuffer().template cast<Vertex>() : std::span<const Vertex>{};
		}

		[[nodiscard]] std::span<const Index> indices() const
		{
			return _content ? indexBuffer().template cast<Index>() : std::span<const Index>{};
		}

		[[nodiscard]] std::size_t vertexCount() const noexcept
		{
			return _content ? _content->vertexBuffer().size() / sizeof(Vertex) : 0;
		}

		[[nodiscard]] std::size_t indexCount() const noexcept
		{
			return _content ? _content->indexBuffer().size() / sizeof(Index) : 0;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return indexCount() == 0;
		}
	};
}
