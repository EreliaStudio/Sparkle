#pragma once

#include "math/perlin/spk_iperlin.hpp"

namespace spk
{
	/**
	 * @class Perlin1D
	 * @brief Specializes the IPerlin noise generator for one-dimensional Perlin noise, suitable for procedural content generation in 1D space.
	 *
	 * Perlin1D extends the IPerlin class to focus specifically on generating one-dimensional Perlin noise. This class simplifies the creation of
	 * natural-looking variations along a single axis, making it ideal for applications like procedural terrain heightmaps (in a simplified 1D context),
	 * noise-based animations, and other graphical effects where only one dimension varies over time or space.
	 *
	 * The class provides a `sample` method for obtaining noise values at specific points along the 1D axis, offering the same extensive customization
	 * options inherited from IPerlin for tailoring noise characteristics. This includes adjusting the frequency, persistance, lacunarity, octave count,
	 * value range, and interpolation method to achieve the desired noise profile.
	 *
	 * Usage Example:
	 * @code
	 * spk::Perlin1D perlinNoise(12345); // Initialize with a seed
	 * perlinNoise.configureFrequency(0.1f);
	 * perlinNoise.configurePersistance(0.5f);
	 * perlinNoise.configureLacunarity(2.0f);
	 * perlinNoise.configureOctave(4);
	 * perlinNoise.configureRange(-1.0f, 1.0f);
	 * perlinNoise.configureInterpolation(spk::IPerlin::Interpolation::SmoothStep);
	 *
	 * float noiseValue = perlinNoise.sample(x); // Sample the noise at a specific point along the 1D axis
	 * @endcode
	 *
	 * @note Perlin1D is tailored for generating noise values in a single dimension, providing a straightforward interface for integrating 1D
	 * Perlin noise into applications. It is designed to offer the same level of customization and control as the more general IPerlin class,
	 * with a focus on one-dimensional noise generation.
	 * 
	 * @see IPerlin
	 */
	class Perlin1D : public IPerlin
	{
	private:
		float _computeWaveLength(const float& p_x, const float& p_frequency) const;
		
	public:
		Perlin1D();
		Perlin1D(const unsigned long& p_seed);
		Perlin1D(const spk::JSON::Object& p_object);

		float sample(const float& p_x) const;
	};
}