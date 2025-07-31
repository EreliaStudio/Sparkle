#include "structure/math/spk_perlin.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace spk
{
	float Perlin::fade(float p_value, Interpolation p_interpolation)
	{
		switch (p_interpolation)
		{
		case Interpolation::SmoothStep:
			return p_value * p_value * (3.f - 2.f * p_value);
		default:
			return p_value;
		}
	}

	float Perlin::lerp(float p_minValue, float p_maxValue, float p_value)
	{
		return p_minValue + (p_maxValue - p_minValue) * p_value;
	}

	float Perlin::grad(int p_hash, float p_x)
	{
		return (p_hash & 1) ? p_x : -p_x;
	}

	float Perlin::grad(int p_hash, float p_x, float p_y)
	{
		return ((p_hash & 1) ? p_x : -p_x) + ((p_hash & 2) ? p_y : -p_y);
	}

	float Perlin::grad(int p_hash, float p_x, float p_y, float p_z)
	{
		return ((p_hash & 1) ? p_x : -p_x) + ((p_hash & 2) ? p_y : -p_y) + ((p_hash & 4) ? p_z : -p_z);
	}

	void Perlin::reseed(unsigned int p_seed)
	{
		std::vector<int> perm(256);
		std::iota(perm.begin(), perm.end(), 0);
		std::shuffle(perm.begin(), perm.end(), std::mt19937(p_seed));
		for (int i = 0; i < 256; ++i)
		{
			_values[i] = _values[i + 256] = perm[i];
		}
	}

	float Perlin::noise1D(float p_x) const
	{
		int clampedX = static_cast<int>(std::floor(p_x)) & 255;
		p_x -= std::floor(p_x);
		float u = fade(p_x, _interpolation);

		int valueA = _values[clampedX];
		int valueB = _values[clampedX + 1];

		return lerp(grad(valueA, p_x), grad(valueB, p_x - 1), u);
	}

	float Perlin::noise2D(float p_x, float p_y) const
	{
		int clampedX = static_cast<int>(std::floor(p_x)) & 255;
		int clampedY = static_cast<int>(std::floor(p_y)) & 255;
		p_x -= std::floor(p_x);
		p_y -= std::floor(p_y);
		float u = fade(p_x, _interpolation);
		float v = fade(p_y, _interpolation);

		int aa = _values[_values[clampedX] + clampedY];
		int ab = _values[_values[clampedX] + clampedY + 1];
		int ba = _values[_values[clampedX + 1] + clampedY];
		int bb = _values[_values[clampedX + 1] + clampedY + 1];

		float res = lerp(lerp(grad(aa, p_x, p_y), grad(ba, p_x - 1, p_y), u), lerp(grad(ab, p_x, p_y - 1), grad(bb, p_x - 1, p_y - 1), u), v);
		return res;
	}

	float Perlin::noise3D(float p_x, float p_y, float p_z) const
	{
		int clampedX = static_cast<int>(std::floor(p_x)) & 255;
		int clampedY = static_cast<int>(std::floor(p_y)) & 255;
		int clampedZ = static_cast<int>(std::floor(p_z)) & 255;
		p_x -= std::floor(p_x);
		p_y -= std::floor(p_y);
		p_z -= std::floor(p_z);
		float u = fade(p_x, _interpolation);
		float v = fade(p_y, _interpolation);
		float w = fade(p_z, _interpolation);

		int aaa = _values[_values[_values[clampedX] + clampedY] + clampedZ];
		int aba = _values[_values[_values[clampedX] + clampedY + 1] + clampedZ];
		int aab = _values[_values[_values[clampedX] + clampedY] + clampedZ + 1];
		int abb = _values[_values[_values[clampedX] + clampedY + 1] + clampedZ + 1];
		int baa = _values[_values[_values[clampedX + 1] + clampedY] + clampedZ];
		int bba = _values[_values[_values[clampedX + 1] + clampedY + 1] + clampedZ];
		int bab = _values[_values[_values[clampedX + 1] + clampedY] + clampedZ + 1];
		int bbb = _values[_values[_values[clampedX + 1] + clampedY + 1] + clampedZ + 1];

		float res = lerp(lerp(lerp(grad(aaa, p_x, p_y, p_z), grad(baa, p_x - 1, p_y, p_z), u),
							  lerp(grad(aba, p_x, p_y - 1, p_z), grad(bba, p_x - 1, p_y - 1, p_z), u),
							  v),
						 lerp(lerp(grad(aab, p_x, p_y, p_z - 1), grad(bab, p_x - 1, p_y, p_z - 1), u),
							  lerp(grad(abb, p_x, p_y - 1, p_z - 1), grad(bbb, p_x - 1, p_y - 1, p_z - 1), u),
							  v),
						 w);
		return res;
	}

	float Perlin::fractal(const Perlin &p_perlin,
						  std::function<float(const Perlin &, float, float, float)> p_noiseFunc,
						  float p_x,
						  float p_y,
						  float p_z,
						  int p_octaves,
						  float p_persistence,
						  float p_lacunarity)
	{
		float sum = 0.f;
		float amplitude = 1.f;
		float freq = 1.f;
		float max = 0.f;

		for (int i = 0; i < p_octaves; ++i)
		{
			sum += p_noiseFunc(p_perlin, p_x * freq, p_y * freq, p_z * freq) * amplitude;
			max += amplitude;
			amplitude *= p_persistence;
			freq *= p_lacunarity;
		}
		return sum / max;
	}

	Perlin::Perlin(unsigned int p_seed, Perlin::Interpolation p_interpolation) :
		_interpolation(p_interpolation)
	{
		reseed(p_seed ? p_seed : std::random_device{}());
	}

	float Perlin::sample1D(float p_x, float p_min, float p_max) const
	{
		float n = fractal(
			*this,
			[](const Perlin &p_values, float p_x, float, float) { return p_values.noise1D(p_x); },
			p_x,
			0,
			0,
			_octaves,
			_persistence,
			_lacunarity);
		return p_min + (p_max - p_min) * (n * 0.5f + 0.5f);
	}

	float Perlin::sample2D(float p_x, float p_y, float p_min, float p_max) const
	{
		float n = fractal(
			*this,
			[](const Perlin &p_values, float p_x, float p_y, float) { return p_values.noise2D(p_x, p_y); },
			p_x,
			p_y,
			0,
			_octaves,
			_persistence,
			_lacunarity);
		return p_min + (p_max - p_min) * (n * 0.5f + 0.5f);
	}

	float Perlin::sample3D(float p_x, float p_y, float p_z, float p_min, float p_max) const
	{
		float n = fractal(
			*this,
			[](const Perlin &p_values, float p_x, float p_y, float p_z) { return p_values.noise3D(p_x, p_y, p_z); },
			p_x,
			p_y,
			p_z,
			_octaves,
			_persistence,
			_lacunarity);
		return p_min + (p_max - p_min) * (n * 0.5f + 0.5f);
	}
}
