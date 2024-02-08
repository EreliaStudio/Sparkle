#pragma once

#include "math/perlin/spk_perlin_1D.hpp"

#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @class Perlin2D
	 * @brief Extends Perlin1D to generate two-dimensional Perlin noise, suitable for a wide range of procedural content generation in 2D space.
	 *
	 * Perlin2D leverages the foundational Perlin noise algorithm provided by Perlin1D, enhancing it to produce noise values across two dimensions.
	 * This class is particularly useful for applications requiring natural-looking textures, terrain generation, or any graphical effects where
	 * variations are needed in both the x and y axes. The ability to produce 2D noise allows for the creation of more complex and detailed procedural
	 * content compared to one-dimensional noise.
	 *
	 * Like its superclass, Perlin2D offers extensive customization options including frequency, persistence, lacunarity, octave count, value range,
	 * and interpolation method. It introduces a `sample` method specifically designed for 2D space, allowing users to obtain noise values at given
	 * x and y coordinates.
	 *
	 * Usage Example:
	 * @code
	 * spk::Perlin2D perlinNoise(12345); // Initialize with a seed
	 * perlinNoise.configureFrequency(0.1f);
	 * perlinNoise.configurePersistance(0.5f);
	 * perlinNoise.configureLacunarity(2.0f);
	 * perlinNoise.configureOctave(4);
	 * perlinNoise.configureRange(-1.0f, 1.0f);
	 * perlinNoise.configureInterpolation(spk::IPerlin::Interpolation::SmoothStep);
	 *
	 * float noiseValue = perlinNoise.sample(x, y); // Sample the noise at specific x, y coordinates
	 * @endcode
	 *
	 * Additionally, Perlin2D provides a templated `sample` method accepting `spk::IVector2<TVectorType>`, enabling integration with SPK's vector
	 * classes for more intuitive positional arguments.
	 *
	 * @note Perlin2D adapts the Perlin noise algorithm for two-dimensional applications, offering a powerful tool for generating detailed and
	 * visually appealing procedural textures and patterns.
	 * 
	 * @see IPerlin, Perlin1D
	 */
	class Perlin2D : public Perlin1D
	{
	private:
		float _computeWaveLength(const float& p_x, const float& p_y, const float& p_frequency) const;

	public:
		/**
		 * @brief Default constructor for the Perlin2D class.
		 * 
		 * Initializes a Perlin2D noise generator with default parameters. This sets up the generator to produce two-dimensional Perlin noise using default seed and noise configuration settings.
		 */
		Perlin2D();

		/**
		 * @brief Constructor for the Perlin2D class with a specified seed.
		 * 
		 * Initializes a Perlin2D noise generator with the specified seed, allowing for reproducible two-dimensional noise patterns. This constructor sets the initial seed for noise generation while retaining the default configuration for other noise parameters.
		 * 
		 * @param p_seed The seed value to use for noise generation.
		 */
		Perlin2D(const unsigned long& p_seed);

		/**
		 * @brief Samples the Perlin noise at specific x and y coordinates.
		 * 
		 * Generates a two-dimensional noise value at the given x and y coordinates based on the current configuration of the noise generator. This method applies the Perlin noise algorithm to produce values that represent the "height" or intensity of the noise at the specified position in 2D space.
		 * 
		 * @param p_x The x coordinate at which to sample the noise.
		 * @param p_y The y coordinate at which to sample the noise.
		 * @return A float representing the noise value at the specified coordinates, within the configured range of the noise generator.
		 */
		float sample(const float& p_x, const float& p_y) const;

		/**
		 * @brief Templated method to sample the Perlin noise using a vector position.
		 * 
		 * Provides a convenient way to sample two-dimensional Perlin noise by passing a vector of any type that implements the `x` and `y` properties. This method simplifies the interface for obtaining noise values when working with SPK's vector classes or custom vector types.
		 * 
		 * @tparam TVectorType The type of the vector (must have `x` and `y` properties accessible).
		 * @param p_position The position vector where the noise should be sampled.
		 * @return A float representing the noise value at the specified vector position.
		 */
		template<typename TVectorType>
		float sample(const spk::IVector2<TVectorType>& p_position) const
		{
			return (sample(static_cast<float>(p_position.x), static_cast<float>(p_position.y)));
		}
	};
}