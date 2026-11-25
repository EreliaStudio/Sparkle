#pragma once

#include <cmath>
#include <stdexcept>

#include "math/perlin.hpp"
#include "math/vector2.hpp"

namespace spk
{
	// Optional coordinate deformation. Independent, deterministic noise fields
	// displace each axis in world units before the caller samples its own field.
	class DomainWarp2D
	{
	private:
		Perlin _xNoise;
		Perlin _yNoise;
		float _amplitude = 0.0f;

	public:
		explicit DomainWarp2D(std::uint32_t seed = 0) :
			_xNoise(seed),
			_yNoise(seed ^ 0x9e3779b9u)
		{
		}

		[[nodiscard]] Perlin &xNoise() noexcept { return _xNoise; }
		[[nodiscard]] const Perlin &xNoise() const noexcept { return _xNoise; }
		[[nodiscard]] Perlin &yNoise() noexcept { return _yNoise; }
		[[nodiscard]] const Perlin &yNoise() const noexcept { return _yNoise; }

		void setAmplitude(float amplitude)
		{
			if (!std::isfinite(amplitude) || amplitude < 0.0f)
			{
				throw std::invalid_argument("Domain warp amplitude must be finite and nonnegative");
			}
			_amplitude = amplitude;
		}

		[[nodiscard]] float amplitude() const noexcept { return _amplitude; }

		[[nodiscard]] Vector2 warp(const Vector2 &position) const
		{
			if (!std::isfinite(position.x) || !std::isfinite(position.y))
			{
				throw std::invalid_argument("Domain warp position must be finite");
			}
			if (_amplitude == 0.0f)
			{
				return position;
			}
			const Vector2 result{
				position.x + _xNoise.sample2D(position.x, position.y, -_amplitude, _amplitude),
				position.y + _yNoise.sample2D(position.x, position.y, -_amplitude, _amplitude)};
			if (!std::isfinite(result.x) || !std::isfinite(result.y))
			{
				throw std::out_of_range("Domain warp position overflow");
			}
			return result;
		}
	};
}
