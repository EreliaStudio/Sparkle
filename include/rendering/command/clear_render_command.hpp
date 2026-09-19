#pragma once

#include <cstdint>

#include "graphics/color.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class ClearRenderCommand final : public RenderCommand
	{
	public:
		enum class Mask : std::uint8_t
		{
			None = 0,
			Color = 1 << 0,
			Depth = 1 << 1,
			Stencil = 1 << 2,
			All = Color | Depth | Stencil
		};

	private:
		Color _color;
		Mask _mask;

	public:
		ClearRenderCommand(Color color, Mask mask);

		void execute(RenderContext &renderContext) const override;

		[[nodiscard]] friend constexpr Mask operator|(Mask lhs, Mask rhs) noexcept
		{
			return static_cast<Mask>(
				static_cast<std::uint8_t>(lhs) |
				static_cast<std::uint8_t>(rhs));
		}

		[[nodiscard]] friend constexpr Mask operator&(Mask lhs, Mask rhs) noexcept
		{
			return static_cast<Mask>(
				static_cast<std::uint8_t>(lhs) &
				static_cast<std::uint8_t>(rhs));
		}
	};
}