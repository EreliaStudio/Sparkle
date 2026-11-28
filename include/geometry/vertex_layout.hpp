#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace spk
{
	class VertexLayout
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

		[[nodiscard]] static std::size_t _typeSize(Attribute::Type type);
		[[nodiscard]] static bool _isIntegerType(Attribute::Type type) noexcept;
		static void _validateAttribute(const Attribute &attribute);

	public:
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
				throw std::overflow_error("VertexLayout padding overflow");
			}
			addPadding(sizeof(T) * count);
		}

		void clear();

		[[nodiscard]] std::size_t stride() const noexcept;
		[[nodiscard]] std::span<const ResolvedAttribute> attributes() const noexcept;
	};
}
