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
		Perlin3D();
		Perlin3D(const unsigned long& p_seed);
		Perlin3D(const spk::JSON::Object& p_object);

		float sample(const float& p_x, const float& p_y, const float& p_z) const;

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