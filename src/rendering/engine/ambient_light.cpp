#include "rendering/engine/ambient_light.hpp"

#include <cmath>

#include "exception.hpp"

namespace spk
{
	RGB AmbientLight::color() const noexcept
	{
		return _color;
	}

	void AmbientLight::setColor(RGB color)
	{
		const auto valid = [](float value) {
			return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
		};
		if (!valid(color.r) || !valid(color.g) || !valid(color.b))
		{
			throw spk::Exception("AmbientLight color must be linear RGB within [0,1]");
		}
		_color = color;
	}

	float AmbientLight::intensity() const noexcept
	{
		return _intensity;
	}

	void AmbientLight::setIntensity(float intensity)
	{
		if (!std::isfinite(intensity) || intensity < 0.0f)
		{
			throw spk::Exception("AmbientLight intensity must be finite and nonnegative");
		}
		_intensity = intensity;
	}

	RGB AmbientLight::contribution() const noexcept
	{
		return {_color.r * _intensity, _color.g * _intensity, _color.b * _intensity};
	}
}
