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

#include "buffer_gpu_resource.hpp"

namespace spk
{
	class VertexBuffer final : public BufferGPUResource
	{
	public:
		enum class Interpretation
		{
			Floating,
			Integer,
			Double
		};

		struct Attribute
		{
			enum class Type
			{
				Byte,
				UnsignedByte,
				Short,
				UnsignedShort,
				Int,
				UnsignedInt,
				HalfFloat,
				Float,
				Double
			};

			std::uint32_t location;
			Type type;
			std::uint32_t componentCount;
			Interpretation interpretation = Interpretation::Floating;
			bool normalized = false;
		};

		struct ResolvedAttribute
		{
			Attribute attribute;
			std::size_t offset;
		};

	private:
		std::vector<ResolvedAttribute> _attributes;
		std::size_t _stride = 0;
		Generation _configurationGeneration = 1;

		[[nodiscard]] static std::size_t _typeSize(Attribute::Type type);
		[[nodiscard]] static bool _isIntegerType(Attribute::Type type) noexcept;
		static void _validateAttribute(const Attribute &attribute);
		void _validateConfigurationEdition() const;
		void _touchConfiguration() noexcept;

		template <typename TVertex>
		void _validateType() const
		{
			static_assert(std::is_trivially_copyable_v<TVertex>, "VertexBuffer requires a trivially copyable type.");
			static_assert(alignof(TVertex) <= alignof(std::max_align_t), "VertexBuffer cannot store an over-aligned type.");
			if (_stride == 0)
			{
				throw std::logic_error("VertexBuffer has no configured layout");
			}
			if (sizeof(TVertex) != _stride)
			{
				throw std::logic_error("Vertex type size does not match the configured VertexBuffer stride");
			}
		}

		[[nodiscard]] static std::size_t _checkedSize(std::size_t count, std::size_t stride);

	protected:
		[[nodiscard]] GLenum _target() const noexcept override;

	public:
		VertexBuffer() = default;

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
		void resize(std::size_t count)
		{
			_validateType<TVertex>();
			_resize(_checkedSize(count, _stride));
		}

		template <typename TVertex>
		void reserve(std::size_t count)
		{
			_validateType<TVertex>();
			_reserve(_checkedSize(count, _stride));
		}

		template <typename TVertex>
		void pushBack(const TVertex &vertex)
		{
			_validateType<TVertex>();
			_append(std::addressof(vertex), sizeof(TVertex));
		}

		template <typename TVertex, typename... TArguments>
		void emplace(TArguments &&...arguments)
		{
			_validateType<TVertex>();
			TVertex vertex(std::forward<TArguments>(arguments)...);
			_append(std::addressof(vertex), sizeof(TVertex));
		}

		template <typename TVertex>
		void insert(std::span<const TVertex> vertices)
		{
			_validateType<TVertex>();
			_append(vertices.data(), vertices.size_bytes());
		}

		template <typename TVertex>
		void insert(std::initializer_list<TVertex> vertices)
		{
			insert<TVertex>(std::span<const TVertex>(vertices.begin(), vertices.size()));
		}

		template <typename TVertex>
		[[nodiscard]] std::span<TVertex> cast()
		{
			_validateType<TVertex>();
			return {reinterpret_cast<TVertex *>(_data()), count()};
		}

		template <typename TVertex>
		[[nodiscard]] std::span<const TVertex> cast() const
		{
			_validateType<TVertex>();
			return {reinterpret_cast<const TVertex *>(_data()), count()};
		}

		[[nodiscard]] std::size_t stride() const noexcept;
		[[nodiscard]] std::size_t count() const noexcept;
		[[nodiscard]] std::span<const ResolvedAttribute> attributes() const noexcept;
		[[nodiscard]] Generation configurationGeneration() const noexcept;
		[[nodiscard]] static GLenum openGLType(Attribute::Type type);
	};
}
