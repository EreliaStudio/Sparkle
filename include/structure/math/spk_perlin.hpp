#pragma once

#include <vector>
#include <array>
#include <random>
#include <functional>

namespace spk
{
	class Perlin
	{
	public:		
		enum class Interpolation
		{
			Linear,
			SmoothStep
		};

	private:
		std::array<int, 512> _values;
		Interpolation _interpolation;
		int _octaves = 3;
		float _persistence = 0.5f;
		float _lacunarity = 2.0f;

		static float fade(float p_value, Interpolation p_interpolation);
		static float lerp(float p_minValue, float p_maxValue, float p_value);
		static float grad(int p_hash, float p_x);
		static float grad(int p_hash, float p_x, float p_y);
		static float grad(int p_hash, float p_x, float p_y, float p_z);

		void reseed(unsigned int p_seed);

		float noise1D(float p_x) const;
		float noise2D(float p_x, float p_y) const;
		float noise3D(float p_x, float p_y, float p_z) const;

		static float fractal(
			const Perlin& p_perlin,
			std::function<float(const Perlin&, float, float, float)> p_noiseFunc,
			float p_x, float p_y, float p_z,
			int p_octaves,
			float p_persistence,
			float p_lacunarity);

	public:
		Perlin(unsigned int p_seed = 0, Perlin::Interpolation p_interp = Perlin::Interpolation::SmoothStep);

		void setInterpolation(Interpolation interp) { _interpolation = interp; }
		void setSeed(unsigned int p_seed) { reseed(p_seed); }
		void setOctaves(int p_oct) { _octaves = p_oct; }
		void setPersistence(float p_p) { _persistence = p_p; }
		void setLacunarity(float p_l) { _lacunarity = p_l; }
			
		const int& octaves() const { return _octaves; }
		const float& persistence() const { return _persistence; }
		const float& lacunarity() const { return _lacunarity; }

		float sample1D(float p_x, float p_min = 0, float p_max = 1) const;
		float sample2D(float p_x, float p_y, float p_min = 0, float p_max = 1) const;
		float sample3D(float p_x, float p_y, float p_z, float p_min = 0, float p_max = 1) const;
	};
}
