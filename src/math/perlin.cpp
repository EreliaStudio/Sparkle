#include "math/perlin.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace spk
{
	float Perlin::_fade(float value, Interpolation interpolation) noexcept
	{
		return interpolation == Interpolation::SmoothStep ? value * value * (3.0f - 2.0f * value) : value;
	}

	float Perlin::_lerp(float from, float to, float alpha) noexcept
	{
		return from + (to - from) * alpha;
	}

	float Perlin::_gradient1D(int hash, float x) noexcept
	{
		return (hash & 1) != 0 ? x : -x;
	}

	float Perlin::_gradient2D(int hash, float x, float y) noexcept
	{
		const float gx = (hash & 1) != 0 ? x : -x;
		const float gy = (hash & 2) != 0 ? y : -y;
		return (gx + gy) * 0.7071067811865475f;
	}

	float Perlin::_gradient3D(int hash, float x, float y, float z) noexcept
	{
		const float gx = (hash & 1) != 0 ? x : -x;
		const float gy = (hash & 2) != 0 ? y : -y;
		const float gz = (hash & 4) != 0 ? z : -z;
		return (gx + gy + gz) * 0.5773502691896258f;
	}

	int Perlin::_lattice(float coordinate) noexcept
	{
		// Modulo before conversion avoids overflowing an int for large finite
		// coordinates. Negative coordinates wrap to the same 256-cell lattice.
		const int wrapped = static_cast<int>(std::fmod(std::floor(coordinate), 256.0f));
		return (wrapped + 256) & 255;
	}

	void Perlin::_validateRange(float minimum, float maximum)
	{
		if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum)
		{
			throw std::invalid_argument("Perlin sample range must be finite and ordered");
		}
	}

	void Perlin::_reseed(std::uint32_t seed) noexcept
	{
		// Explicit Fisher-Yates and SplitMix64 produce the same permutation
		// across standard library implementations (unlike std::shuffle).
		std::array<int, 256> values{};
		for (std::size_t index = 0; index < values.size(); ++index)
		{
			values[index] = static_cast<int>(index);
		}

		std::uint64_t state = seed;
		const auto next = [&state]() noexcept {
			state += 0x9E3779B97F4A7C15ULL;
			std::uint64_t value = state;
			value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
			value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
			return static_cast<std::uint32_t>((value ^ (value >> 31)) >> 32);
		};

		for (std::size_t count = values.size(); count > 1; --count)
		{
			std::swap(values[count - 1], values[next() % count]);
		}
		for (std::size_t index = 0; index < values.size(); ++index)
		{
			_permutation[index] = values[index];
			_permutation[index + values.size()] = values[index];
		}
		_seed = seed;
	}

	float Perlin::_noise1D(float x) const noexcept
	{
		const int latticeX = _lattice(x);
		const float dx = x - std::floor(x);
		const float u = _fade(dx, _interpolation);
		return _lerp(
			_gradient1D(_permutation[latticeX], dx),
			_gradient1D(_permutation[latticeX + 1], dx - 1.0f),
			u);
	}

	float Perlin::_noise2D(float x, float y) const noexcept
	{
		const int ix = _lattice(x);
		const int iy = _lattice(y);
		const float dx = x - std::floor(x);
		const float dy = y - std::floor(y);
		const float u = _fade(dx, _interpolation);
		const float v = _fade(dy, _interpolation);

		const int aa = _permutation[_permutation[ix] + iy];
		const int ab = _permutation[_permutation[ix] + iy + 1];
		const int ba = _permutation[_permutation[ix + 1] + iy];
		const int bb = _permutation[_permutation[ix + 1] + iy + 1];

		return _lerp(
			_lerp(_gradient2D(aa, dx, dy), _gradient2D(ba, dx - 1.0f, dy), u),
			_lerp(_gradient2D(ab, dx, dy - 1.0f), _gradient2D(bb, dx - 1.0f, dy - 1.0f), u),
			v);
	}

	float Perlin::_noise3D(float x, float y, float z) const noexcept
	{
		const int ix = _lattice(x);
		const int iy = _lattice(y);
		const int iz = _lattice(z);
		const float dx = x - std::floor(x);
		const float dy = y - std::floor(y);
		const float dz = z - std::floor(z);
		const float u = _fade(dx, _interpolation);
		const float v = _fade(dy, _interpolation);
		const float w = _fade(dz, _interpolation);

		const int aaa = _permutation[_permutation[_permutation[ix] + iy] + iz];
		const int aba = _permutation[_permutation[_permutation[ix] + iy + 1] + iz];
		const int aab = _permutation[_permutation[_permutation[ix] + iy] + iz + 1];
		const int abb = _permutation[_permutation[_permutation[ix] + iy + 1] + iz + 1];
		const int baa = _permutation[_permutation[_permutation[ix + 1] + iy] + iz];
		const int bba = _permutation[_permutation[_permutation[ix + 1] + iy + 1] + iz];
		const int bab = _permutation[_permutation[_permutation[ix + 1] + iy] + iz + 1];
		const int bbb = _permutation[_permutation[_permutation[ix + 1] + iy + 1] + iz + 1];

		return _lerp(
			_lerp(
				_lerp(_gradient3D(aaa, dx, dy, dz), _gradient3D(baa, dx - 1.0f, dy, dz), u),
				_lerp(_gradient3D(aba, dx, dy - 1.0f, dz), _gradient3D(bba, dx - 1.0f, dy - 1.0f, dz), u),
				v),
			_lerp(
				_lerp(_gradient3D(aab, dx, dy, dz - 1.0f), _gradient3D(bab, dx - 1.0f, dy, dz - 1.0f), u),
				_lerp(
					_gradient3D(abb, dx, dy - 1.0f, dz - 1.0f),
					_gradient3D(bbb, dx - 1.0f, dy - 1.0f, dz - 1.0f),
					u),
				v),
			w);
	}

	float Perlin::_fractal(float x, float y, float z, int dimensions) const
	{
		double total = 0.0;
		double amplitude = 1.0;
		double amplitudeSum = 0.0;
		double frequency = _frequency;
		const double limit = std::numeric_limits<float>::max();

		for (int octave = 0; octave < _octaves; ++octave)
		{
			const double sx = static_cast<double>(x) * frequency;
			const double sy = static_cast<double>(y) * frequency;
			const double sz = static_cast<double>(z) * frequency;
			if (!std::isfinite(sx) || !std::isfinite(sy) || !std::isfinite(sz) ||
				std::abs(sx) > limit || std::abs(sy) > limit || std::abs(sz) > limit)
			{
				throw std::out_of_range("Perlin octave coordinates overflow float range");
			}

			float value = 0.0f;
			if (dimensions == 1)
			{
				value = _noise1D(static_cast<float>(sx));
			}
			else if (dimensions == 2)
			{
				value = _noise2D(static_cast<float>(sx), static_cast<float>(sy));
			}
			else
			{
				value = _noise3D(static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(sz));
			}
			total += static_cast<double>(value) * amplitude;
			amplitudeSum += amplitude;

			if (octave + 1 != _octaves)
			{
				amplitude *= _persistence;
				frequency *= _lacunarity;
				if (!std::isfinite(amplitude) || !std::isfinite(frequency) ||
					!std::isfinite(amplitudeSum))
				{
					throw std::out_of_range("Perlin fractal parameters overflow");
				}
			}
		}
		return static_cast<float>(total / amplitudeSum);
	}

	float Perlin::_mapToRange(float value, float minimum, float maximum) noexcept
	{
		const double normalized = std::clamp(static_cast<double>(value) * 0.5 + 0.5, 0.0, 1.0);
		const double mapped = static_cast<double>(minimum) +
							  (static_cast<double>(maximum) - minimum) * normalized;
		return static_cast<float>(std::clamp(mapped, static_cast<double>(minimum), static_cast<double>(maximum)));
	}

	Perlin::Perlin(std::uint32_t seed, Interpolation interpolation)
	{
		setInterpolation(interpolation);
		_reseed(seed);
	}

	void Perlin::setSeed(std::uint32_t seed) noexcept
	{
		_reseed(seed);
	}

	void Perlin::setInterpolation(Interpolation interpolation)
	{
		if (interpolation != Interpolation::Linear && interpolation != Interpolation::SmoothStep)
		{
			throw std::invalid_argument("Unknown Perlin interpolation mode");
		}
		_interpolation = interpolation;
	}

	void Perlin::setOctaves(int count)
	{
		if (count < 1 || count > 32)
		{
			throw std::invalid_argument("Perlin octaves must be in [1, 32]");
		}
		_octaves = count;
	}

	void Perlin::setPersistence(float persistence)
	{
		if (!std::isfinite(persistence) || persistence < 0.0f)
		{
			throw std::invalid_argument("Perlin persistence must be finite and non-negative");
		}
		_persistence = persistence;
	}

	void Perlin::setLacunarity(float lacunarity)
	{
		if (!std::isfinite(lacunarity) || lacunarity <= 0.0f)
		{
			throw std::invalid_argument("Perlin lacunarity must be finite and positive");
		}
		_lacunarity = lacunarity;
	}

	void Perlin::setFrequency(float frequency)
	{
		if (!std::isfinite(frequency) || frequency <= 0.0f)
		{
			throw std::invalid_argument("Perlin frequency must be finite and positive");
		}
		_frequency = frequency;
	}

	std::uint32_t Perlin::seed() const noexcept
	{
		return _seed;
	}

	Perlin::Interpolation Perlin::interpolation() const noexcept
	{
		return _interpolation;
	}

	int Perlin::octaves() const noexcept
	{
		return _octaves;
	}

	float Perlin::persistence() const noexcept
	{
		return _persistence;
	}

	float Perlin::lacunarity() const noexcept
	{
		return _lacunarity;
	}

	float Perlin::frequency() const noexcept
	{
		return _frequency;
	}

	float Perlin::sample1D(float x, float minimum, float maximum) const
	{
		_validateRange(minimum, maximum);
		if (!std::isfinite(x))
		{
			throw std::invalid_argument("Perlin sample coordinates must be finite");
		}
		return _mapToRange(_fractal(x, 0.0f, 0.0f, 1), minimum, maximum);
	}

	float Perlin::sample2D(float x, float y, float minimum, float maximum) const
	{
		_validateRange(minimum, maximum);
		if (!std::isfinite(x) || !std::isfinite(y))
		{
			throw std::invalid_argument("Perlin sample coordinates must be finite");
		}
		return _mapToRange(_fractal(x, y, 0.0f, 2), minimum, maximum);
	}

	float Perlin::sample3D(float x, float y, float z, float minimum, float maximum) const
	{
		_validateRange(minimum, maximum);
		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
		{
			throw std::invalid_argument("Perlin sample coordinates must be finite");
		}
		return _mapToRange(_fractal(x, y, z, 3), minimum, maximum);
	}
}
