#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "geometry/vertex_layout.hpp"
#include "graphics/opengl/buffer_gpu_resource.hpp"

namespace spk
{
	class VertexBuffer final : public BufferGPUResource
	{
	public:
		using Interpretation = VertexLayout::Interpretation;
		using Attribute = VertexLayout::Attribute;
		using ResolvedAttribute = VertexLayout::ResolvedAttribute;

	public:
		class State final : public BufferGPUResource::State
		{
			friend class VertexBuffer;

		private:
			VertexLayout _layout;
			Generation _configurationGeneration = 1;

		protected:
			[[nodiscard]] std::unique_ptr<GPUResource::State> _clone() const override
			{
				return std::make_unique<State>(*this);
			}
			[[nodiscard]] GLenum _target() const noexcept override;
			void _validateBuffer(const Buffer &buffer) const override;

		public:
			[[nodiscard]] std::size_t stride() const noexcept
			{
				return _layout.stride();
			}
			[[nodiscard]] std::span<const ResolvedAttribute> attributes() const noexcept
			{
				return _layout.attributes();
			}
			[[nodiscard]] Generation configurationGeneration() const noexcept
			{
				return _configurationGeneration;
			}
		};

		using Handle = GPUResource::Handle<State>;

	private:
		void _validateConfigurationEdition() const;
		void _touchConfiguration() noexcept;

		template <typename TVertex>
		void _validateType() const
		{
			static_assert(std::is_trivially_copyable_v<TVertex>, "VertexBuffer requires a trivially copyable type.");
			static_assert(alignof(TVertex) <= alignof(std::max_align_t), "VertexBuffer cannot store an over-aligned type.");
			const auto &content = state<State>();
			if (content._layout.stride() == 0)
			{
				throw std::logic_error("VertexBuffer has no configured layout");
			}
			if (sizeof(TVertex) != content._layout.stride())
			{
				throw std::logic_error("Vertex type size does not match the configured VertexBuffer stride");
			}
		}

	public:
		VertexBuffer();
		[[nodiscard]] std::unique_ptr<GPUResource> clone() const override
		{
			auto result = std::make_unique<VertexBuffer>(*this);
			result->_setState(_cloneState());
			return result;
		}
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		void setLayout(const VertexLayout &layout);
		void addAttribute(Attribute attribute);
		void addAttribute(
			std::uint32_t location,
			Attribute::Type type,
			std::uint32_t componentCount,
			Interpretation interpretation = Interpretation::Floating,
			bool normalized = false);
		void addPadding(std::size_t byteCount);

		template <typename T>
		void addPadding(std::size_t count)
		{
			if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
			{
				throw std::overflow_error("VertexBuffer padding overflow");
			}
			addPadding(sizeof(T) * count);
		}

		void clearConfiguration();

		template <typename TVertex>
		[[nodiscard]] std::span<const TVertex> cast() const
		{
			_validateType<TVertex>();
			return buffer() != nullptr ? buffer()->cast<TVertex>() : std::span<const TVertex>{};
		}

		[[nodiscard]] std::size_t stride() const noexcept;
		[[nodiscard]] std::size_t count() const noexcept;
		[[nodiscard]] std::span<const ResolvedAttribute> attributes() const noexcept;
		[[nodiscard]] Generation configurationGeneration() const noexcept;
		[[nodiscard]] static GLenum openGLType(Attribute::Type type);
	};
}
