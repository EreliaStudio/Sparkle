#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>

#include "graphics/opengl/layout_buffer.hpp"

namespace spk
{
	template <typename TVertexType>
	class Mesh
	{
	public:
		using Vertex = TVertexType;
		using Index = std::uint32_t;

	private:
		struct Content
		{
			LayoutBuffer layout;
		};

		std::shared_ptr<const Content> _content;

	protected:
		void _attachContent(std::shared_ptr<const Content> content)
		{
			_content = std::move(content);
		}

	public:
		template <typename TMeshType>
		class Builder
		{
		private:
			std::shared_ptr<Content> _content;
			bool _attributesInitialized = false;

			void _check() const
			{
				if (_content == nullptr)
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

				_setupAttributes(_content->layout.vertexBuffer());
				_attributesInitialized = true;
			}

		protected:
			virtual void _setupAttributes(VertexBuffer &) = 0;
			[[nodiscard]] virtual std::size_t _vertexCount() const
			{
				return _content->layout.vertexBuffer().count();
			}
			virtual void _setupMesh(TMeshType &mesh)
			{
			}

		public:
			Builder() :
				_content(std::make_shared<Content>())
			{
				_content->layout.indexBuffer().setType(IndexBuffer::Type::UnsignedInt);
			}

			virtual ~Builder() = default;


			[[nodiscard]] std::size_t vertexCount() const
			{
				_check();
				return _vertexCount();
			}

			[[nodiscard]] Index addVertex(const Vertex &vertex)
			{
				_check();
				_initializeAttributes();

				const std::size_t vertexCount = _vertexCount();

				if (vertexCount > std::numeric_limits<Index>::max())
				{
					throw std::overflow_error("Mesh vertex index overflow");
				}

				_content->layout.vertexBuffer().pushBack(vertex);

				return static_cast<Index>(vertexCount);
			}

			// Batch writes avoid per-element GPU buffer state validation and resizing.
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
				_content->layout.vertexBuffer().insert(vertices);
				return static_cast<Index>(first);
			}


			// Append a contiguous batch, then modify only the appended region in place.
			// This retains the bulk copy while avoiding a separate output container.
			template <typename TModifier>
				requires std::invocable<TModifier, std::span<Vertex>>
			[[nodiscard]] Index addVertices(std::span<const Vertex> vertices, TModifier &&modifier)
			{
				const Index first = addVertices(vertices);
				auto appended = _content->layout.vertexBuffer().template cast<Vertex>().subspan(first, vertices.size());
				std::invoke(std::forward<TModifier>(modifier), appended);
				return first;
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

				auto &buffer = _content->layout.vertexBuffer();
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
				const auto count = _content->layout.vertexBuffer().count();
				if (std::any_of(indices.begin(), indices.end(), [count](Index index) {
						return index >= count;
					}))
				{
					throw std::out_of_range("Mesh index references a missing vertex");
				}
				_content->layout.indexBuffer().insert(indices);
			}

			// Avoid an intermediate allocation when each vertex is already one triangle-list index.
			void addSequentialIndices(std::size_t count)
			{
				_check();
				if (count > _vertexCount() || count > std::numeric_limits<Index>::max())
				{
					throw std::out_of_range("Mesh sequential index references a missing vertex");
				}

				auto &buffer = _content->layout.indexBuffer();
				const std::size_t previousCount = buffer.count();
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

				if (index >= _content->layout.vertexBuffer().count())
				{
					throw std::out_of_range("Mesh index references a missing vertex");
				}

				_content->layout.indexBuffer().pushBack(index);
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

				_content->layout.vertexBuffer().template reserve<Vertex>(vertexCount);
				_content->layout.indexBuffer().template reserve<Index>(indexCount);
			}

			template <typename... TArgs>
				requires std::constructible_from<TMeshType, TArgs...>
			[[nodiscard]] TMeshType build(TArgs &&...args) &&
			{
				_check();
				_initializeAttributes();

				_content->layout.validate();

				TMeshType result(std::forward<TArgs>(args)...);

				result._attachContent(std::exchange(_content, nullptr));

				_setupMesh(result);

				return result;
			}
		};

		Mesh() = default;

		[[nodiscard]] const LayoutBuffer &layout() const
		{
			if (_content == nullptr)
			{
				throw std::logic_error("Mesh is empty");
			}

			return _content->layout;
		}

		[[nodiscard]] std::size_t vertexCount() const noexcept
		{
			return _content ? _content->layout.vertexBuffer().count() : 0;
		}

		[[nodiscard]] std::size_t indexCount() const noexcept
		{
			return _content ? _content->layout.indexBuffer().count() : 0;
		}

		[[nodiscard]] IndexBuffer::Type indexType() const noexcept
		{
			return IndexBuffer::Type::UnsignedInt;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return indexCount() == 0;
		}
	};
}
