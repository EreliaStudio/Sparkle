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

	protected:
			/**
		 * @brief Executes the sampling process for generating Perlin noise.
		 *
		 * This function is a core component of the Perlin noise generation process, orchestrating the sampling based on the provided lambda function.
		 * It allows for a flexible implementation by accepting a lambda function that dictates the specific sampling behavior, facilitating the
		 * adaptation of the sampling process to different contexts and noise characteristics.
		 *
		 * @param p_lambda A lambda function that takes a frequency and returns a float, defining the sampling behavior for the noise generation.
		 * @return The resulting noise value from the executed sampling process.
		 */
		float _executeSample(const std::function<float(const float& p_frequency)>& p_lambda) const;

		/**
		 * @brief Calculates the dot product of the grid's gradient vector and the distance vector in 3D space.
		 *
		 * This function is pivotal in generating 3D Perlin noise, where it computes the dot product between the gradient vector at a grid point
		 * and the vector from that grid point to the sample point. This calculation is essential for interpolating the noise values and producing
		 * the characteristic Perlin noise effect.
		 *
		 * @param ix The x-coordinate of the grid point.
		 * @param iy The y-coordinate of the grid point.
		 * @param iz The z-coordinate of the grid point.
		 * @param x The x-coordinate of the sample point.
		 * @param y The y-coordinate of the sample point.
		 * @param z The z-coordinate of the sample point.
		 * @return The dot product result, contributing to the noise value calculation.
		 */
		float _dotGridGradient(const int& ix, const int& iy, const int& iz, const float& x, const float& y, const float& z) const;

		/**
		 * @brief Calculates the dot product of the grid's gradient vector and the distance vector in 2D space.
		 *
		 * Similar to its 3D counterpart, this function is crucial for generating 2D Perlin noise. It calculates the dot product between the
		 * gradient vector at a 2D grid point and the distance vector to the sample point, aiding in the interpolation and generation of noise values.
		 *
		 * @param ix The x-coordinate of the grid point.
		 * @param iy The y-coordinate of the grid point.
		 * @param x The x-coordinate of the sample point.
		 * @param y The y-coordinate of the sample point.
		 * @return The dot product result, used in the 2D noise value calculation.
		 */
		float _dotGridGradient(const int& ix, const int& iy, const float& x, const float& y) const;

		/**
		 * @brief Calculates the dot product of the grid's gradient vector and the distance vector in 1D space.
		 *
		 * This function is essential for generating 1D Perlin noise, where it computes the dot product between the gradient vector at a grid point
		 * in 1D and the distance to the sample point. It facilitates the interpolation of noise values, integral to the Perlin noise effect.
		 *
		 * @param ix The coordinate of the grid point.
		 * @param x The coordinate of the sample point.
		 * @return The dot product result, used in the 1D noise value calculation.
		 */
		float _dotGridGradient(const int& ix, const float& x) const;

		/**
		 * @brief Interpolates between two values based on a weight.
		 *
		 * This function performs the interpolation between two values (a0 and a1) based on a weight (w), which dictates the proportion of each value
		 * in the final result. This interpolation is fundamental in blending noise values smoothly across the noise field.
		 *
		 * @param a0 The first value to interpolate.
		 * @param a1 The second value to interpolate.
		 * @param w The weight influencing the interpolation, typically a value between 0 and 1.
		 * @return The interpolated value between a0 and a1.
		 */
		float _interpolate(const float& a0, const float& a1, const float& w) const;

		/**
		 * @brief Calculates the smoothing ratio for a given weight.
		 *
		 * This function calculates a smoothing ratio for the interpolation, enhancing the visual continuity and smoothness of the noise. It applies
		 * a smoothstep function (or similar) to the weight, which adjusts the transition between noise values, improving the natural appearance of the noise.
		 *
		 * @param w The weight value, typically between 0 and 1, used to compute the smoothing ratio.
		 * @return The calculated smoothing ratio, influencing the smoothness of the noise transition.
		 */
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
		 * @brief Overloaded constructor for the IPerlin class that takes a JSON object.
		 * @param p_object JSON object containing configuration for Perlin noise generation.
		 */
		IPerlin(const spk::JSON::Object& p_object);

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
