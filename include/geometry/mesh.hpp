#pragma once

#include <memory>

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

			[[nodiscard]] Index addVertex(const Vertex &vertex)
			{
				_check();
				_initializeAttributes();

				const Index index = _content->layout.vertexBuffer().count();

				if (index > std::numeric_limits<Index>::max())
				{
					throw std::overflow_error("Mesh vertex index overflow");
				}

				_content->layout.vertexBuffer().pushBack(vertex);

				return static_cast<Index>(index);
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
