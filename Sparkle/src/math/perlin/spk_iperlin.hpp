#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"
#include <functional>

namespace spk
{
	/**
	 * @brief Provides a flexible implementation of Perlin noise generation, suitable for various procedural content generation tasks.
	 *
	 * This class encapsulates the algorithm for generating Perlin noise, a type of gradient noise often used in procedural texturing,
	 * terrain generation, and other graphical applications requiring natural-looking variations. It offers extensive customization options,
	 * including seed configuration, frequency, persistance, lacunarity, octave count, value range, and interpolation method, enabling fine-tuned
	 * control over the noise output.
	 *
	 * The class supports multiple dimensions of noise generation (1D, 2D, and 3D) and provides two interpolation methods: Linear and SmoothStep,
	 * allowing users to choose the smoothness of the transitions between noise values. The internal implementation uses a permutation table for
	 * gradient vector lookup and a predefined set of gradient vectors to compute dot products, crucial for the generation of Perlin noise.
	 *
	 * Usage Example:
	 * @code
	 * spk::IPerlin perlinNoise(12345); // Initialize with a seed
	 * perlinNoise.configureFrequency(0.1f);
	 * perlinNoise.configurePersistance(0.5f);
	 * perlinNoise.configureLacunarity(2.0f);
	 * perlinNoise.configureOctave(4);
	 * perlinNoise.configureRange(-1.0f, 1.0f);
	 * perlinNoise.configureInterpolation(spk::IPerlin::Interpolation::SmoothStep);
	 *
	 * float noiseValue = perlinNoise.sample(x, y, z); // Sample the noise at a specific point
	 * @endcode
	 */
	class IPerlin
	{
	public:
		enum class Interpolation
		{
			Linear,
			SmoothStep
			
		};

	private:
		Interpolation _interpolation = Interpolation::Linear;

		static const size_t PermutationTableSize = 256;

		using PermutationTable = unsigned char[PermutationTableSize * 2];

		PermutationTable _permutationTable;

		const float _gradients[12][3] = {
			{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
			{1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
			{0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
		};

		unsigned long _seed = 12500;
		float _min = 0;
		float _max = 100;
		float _range = 100;
		size_t _octaveValue = 3;
		float _frequency = 50.0f;
		float _persistance = 0.5f;
		float _lacunarity = 2.0f;

		void _onSeedEdition();

		float _executeSample(const std::function<float(const float& p_frequency)>& p_lambda) const;

		float _dotGridGradient(const int& ix, const int& iy, const int& iz, const float& x, const float& y, const float& z) const;
		float _dotGridGradient(const int& ix, const int& iy, const float& x, const float& y) const;
		float _dotGridGradient(const int& ix, const float& x) const;

		float _interpolate(const float& a0, const float& a1, const float& w) const;
		float _calcRatio(const float& w) const;

	public:
		IPerlin();
		IPerlin(unsigned long p_seed);
		IPerlin(const spk::JSON::Object& p_object);

		const unsigned long &seed() const;

		void configureSeed(unsigned long p_seed);
		void configureFrequency(float p_frequency);
		void configurePersistance(float p_persistance);
		void configureLacunarity(float p_lacunarity);
		void configureOctave(size_t p_octaveValue);
		void configureRange(float p_min, float p_max);
		void configureInterpolation(Interpolation p_interpolation);
	};
}
