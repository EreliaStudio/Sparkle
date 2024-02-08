#pragma once

#include "math/perlin/spk_perlin_2D.hpp"

#include "math/spk_vector3.hpp"

namespace spk
{
	/**
	 * @class Perlin3D
	 * @brief Enhances Perlin2D by generating three-dimensional Perlin noise, ideal for volumetric procedural content generation.
	 *
	 * Perlin3D extends the capabilities of Perlin2D to three dimensions, allowing for the creation of complex, naturally
	 * varying noise patterns in 3D space. This class is particularly useful for generating textures for 3D models,
	 * volumetric fog, terrain generation in 3D games, or any application requiring procedural variation throughout a
	 * three-dimensional volume.
	 *
	 * Building upon the foundation set by Perlin1D and Perlin2D, Perlin3D inherits the extensive customization options
	 * for noise generation, including the ability to adjust frequency, persistence, lacunarity, octave count, and the value
	 * range. It introduces a method for sampling noise values given three coordinates (x, y, z), facilitating the generation
	 * of detailed and cohesive procedural textures and effects in a 3D environment.
	 *
	 * Usage Example:
	 * @code
	 * spk::Perlin3D perlinNoise(12345); // Initialize with a seed
	 * perlinNoise.configureFrequency(0.05f);
	 * perlinNoise.configurePersistance(0.5f);
	 * perlinNoise.configureLacunarity(2.0f);
	 * perlinNoise.configureOctave(4);
	 * perlinNoise.configureRange(-1.0f, 1.0f);
	 * perlinNoise.configureInterpolation(spk::IPerlin::Interpolation::SmoothStep);
	 *
	 * float noiseValue = perlinNoise.sample(x, y, z); // Sample the noise at specific x, y, z coordinates
	 * @endcode
	 *
	 * Additionally, Perlin3D provides a templated `sample` method that accepts `spk::IVector3<TVectorType>`, seamlessly integrating with SPK's vector class for 3D coordinates. This facilitates more intuitive usage within 3D applications and simulations.
	 *
	 * @note Perlin3D offers a powerful tool for 3D procedural content creation, enabling developers to imbue their virtual environments with realistic and visually appealing noise-based textures and patterns.
	 * 
	 * @see IPerlin, Perlin2D, Perlin1D
	 */
	class Perlin3D : public Perlin2D
	{
	private:
		float _computeWaveLength(const float& p_x, const float& p_y, const float& p_z, const float& p_frequency) const;

	public:
		/**
		 * @brief Default constructor for the Perlin3D class.
		 * 
		 * Initializes a Perlin3D noise generator with default parameters. This sets up the generator to produce three-dimensional Perlin noise using the default seed and noise configuration settings.
		 */
		Perlin3D();

		/**
		 * @brief Constructor for the Perlin3D class with a specified seed.
		 * 
		 * Initializes a Perlin3D noise generator with the specified seed, allowing for reproducible three-dimensional noise patterns. This constructor sets the initial seed for noise generation while retaining the default configuration for other noise parameters.
		 * 
		 * @param p_seed The seed value to use for noise generation.
		 */
		Perlin3D(const unsigned long& p_seed);

		/**
		 * @brief Samples the Perlin noise at specific x, y, and z coordinates.
		 * 
		 * Generates a three-dimensional noise value at the given x, y, and z coordinates based on the current configuration of the noise generator. This method applies the Perlin noise algorithm to produce values that represent the "height" or intensity of the noise at the specified position in 3D space.
		 * 
		 * @param p_x The x coordinate at which to sample the noise.
		 * @param p_y The y coordinate at which to sample the noise.
		 * @param p_z The z coordinate at which to sample the noise.
		 * @return A float representing the noise value at the specified coordinates, within the configured range of the noise generator.
		 */
		float sample(const float& p_x, const float& p_y, const float& p_z) const;

		/**
		 * @brief Templated method to sample the Perlin noise using a 3D vector position.
		 * 
		 * Provides a convenient way to sample three-dimensional Perlin noise by passing a vector of any type that implements the `x`, `y`, and `z` properties. This method simplifies the interface for obtaining noise values when working with SPK's vector classes or custom vector types for 3D coordinates.
		 * 
		 * @tparam TVectorType The type of the vector (must have `x`, `y`, and `z` properties accessible).
		 * @param p_position The position vector where the noise should be sampled.
		 * @return A float representing the noise value at the specified vector position.
		 */
		template<typename TVectorType>
		float sample(const spk::IVector3<TVectorType>& p_position) const
		{
			return (sample(
					static_cast<float>(p_position.x),
					static_cast<float>(p_position.y),
					static_cast<float>(p_position.z))
				);
		}
	};
}