#pragma once

#include "graphics/color.hpp"

namespace spk
{
	class AmbientLight final
	{
	private:
		RGB _color{1.0f, 1.0f, 1.0f};
		float _intensity = 0.0f;

	public:
		[[nodiscard]] RGB color() const noexcept;
		void setColor(RGB color);
		[[nodiscard]] float intensity() const noexcept;
		void setIntensity(float intensity);
		[[nodiscard]] RGB contribution() const noexcept;
	};
}
