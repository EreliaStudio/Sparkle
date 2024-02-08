#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"
#include <functional>

namespace spk
{
	/**
	 * @interface IPerlin
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
		/**
		 * @enum Interpolation
		 * @brief Defines the interpolation methods available for noise generation.
		 * 
		 * Specifies how the intermediate values between noise points are calculated, affecting the smoothness of the noise.
		 * - Linear: Produces a straight-line interpolation between points, resulting in a somewhat harsh transition.
		 * - SmoothStep: Uses a smoother step function to interpolate between points, producing smoother transitions.
		 */
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
		/**
		 * @brief Default constructor for the IPerlin class.
		 * 
		 * Initializes a Perlin noise generator with default parameters. The default seed is used for the noise generation.
		 */
		IPerlin();

		/**
		 * @brief Constructor for the IPerlin class with a specified seed.
		 * 
		 * Initializes a Perlin noise generator with the specified seed, allowing for reproducible noise patterns.
		 * 
		 * @param p_seed The seed value to use for noise generation.
		 */
		IPerlin(unsigned long p_seed);

		/**
		 * @brief Gets the current seed used for noise generation.
		 * 
		 * @return The current seed value.
		 */
		const unsigned long &seed() const;

		/**
		 * @brief Configures the seed used for noise generation.
		 * 
		 * Sets the seed value for the Perlin noise generator, allowing for different noise patterns.
		 * 
		 * @param p_seed The seed value to use for noise generation.
		 */
		void configureSeed(unsigned long p_seed);

		/**
		 * @brief Configures the frequency of the noise.
		 * 
		 * Sets the frequency of the noise, affecting the scale of the noise patterns.
		 * 
		 * @param p_frequency The frequency value for the noise.
		 */
		void configureFrequency(float p_frequency);

		/**
		 * @brief Configures the persistence of the noise.
		 * 
		 * Sets the persistence value, affecting the amplitude of each octave in the noise generation.
		 * 
		 * @param p_persistance The persistence value for the noise.
		 */
		void configurePersistance(float p_persistance);

		/**
		 * @brief Configures the lacunarity of the noise.
		 * 
		 * Sets the lacunarity value, affecting the frequency of each octave in the noise generation.
		 * 
		 * @param p_lacunarity The lacunarity value for the noise.
		 */
		void configureLacunarity(float p_lacunarity);

		/**
		 * @brief Configures the number of octaves used in noise generation.
		 * 
		 * Sets the number of octaves, affecting the level of detail in the noise pattern.
		 * 
		 * @param p_octaveValue The number of octaves to use for noise generation.
		 */
		void configureOctave(size_t p_octaveValue);

		/**
		 * @brief Configures the range of the noise output values.
		 * 
		 * Sets the minimum and maximum values for the noise output, allowing for control over the value range of the noise.
		 * 
		 * @param p_min The minimum value of the noise output.
		 * @param p_max The maximum value of the noise output.
		 */
		void configureRange(float p_min, float p_max);

		/**
		 * @brief Configures the interpolation method used in noise generation.
		 * 
		 * Sets the interpolation method, affecting how intermediate noise values are calculated between known points.
		 * 
		 * @param p_interpolation The interpolation method to use (Linear or SmoothStep).
		 */
		void configureInterpolation(Interpolation p_interpolation);
	};
}
