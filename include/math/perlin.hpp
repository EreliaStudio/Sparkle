#pragma once

#include <array>
#include <cstdint>

namespace spk
{
	// Seeded, continuous gradient noise. Samples depend only on the instance's
	// configuration and the coordinates, never on sampling order or thread state.
	// The lattice repeats every 256 units at each octave.
	class Perlin
	{
	public:
		enum class Interpolation
		{
			Linear,
			SmoothStep,
			Quintic
		};

	private:
		std::array<int, 512> _permutation{};
		std::uint32_t _seed = 0;
		Interpolation _interpolation = Interpolation::Quintic;
		int _octaves = 3;
		float _persistence = 0.5f;
		float _lacunarity = 2.0f;
		float _frequency = 1.0f;

		static float _fade(float value, Interpolation interpolation) noexcept;
		static float _lerp(float from, float to, float alpha) noexcept;
		static float _gradient1D(int hash, float x) noexcept;
		static float _gradient2D(int hash, float x, float y) noexcept;
		static float _gradient3D(int hash, float x, float y, float z) noexcept;
		static int _lattice(float coordinate) noexcept;
		static void _validateRange(float minimum, float maximum);
		void _reseed(std::uint32_t seed) noexcept;

		[[nodiscard]] float _noise1D(float x) const noexcept;
		[[nodiscard]] float _noise2D(float x, float y) const noexcept;
		[[nodiscard]] float _noise3D(float x, float y, float z) const noexcept;
		[[nodiscard]] float _fractal(float x, float y, float z, int dimensions) const;
		[[nodiscard]] static float _mapToRange(float value, float minimum, float maximum) noexcept;

	public:
		// Unlike SparkleV1, seed 0 is deterministic (no random_device fallback).
		explicit Perlin(std::uint32_t seed = 0, Interpolation interpolation = Interpolation::Quintic);

		void setSeed(std::uint32_t seed) noexcept;
		void setInterpolation(Interpolation interpolation);
		void setOctaves(int count);
		void setPersistence(float persistence);
		void setLacunarity(float lacunarity);
		void setFrequency(float frequency);

		[[nodiscard]] std::uint32_t seed() const noexcept;
		[[nodiscard]] Interpolation interpolation() const noexcept;
		[[nodiscard]] int octaves() const noexcept;
		[[nodiscard]] float persistence() const noexcept;
		[[nodiscard]] float lacunarity() const noexcept;
		[[nodiscard]] float frequency() const noexcept;

		// Sample normalized fractal noise remapped to [minimum, maximum].
		// Coordinates and bounds must be finite, with minimum <= maximum.
		[[nodiscard]] float sample1D(float x, float minimum = 0.0f, float maximum = 1.0f) const;
		[[nodiscard]] float sample2D(float x, float y, float minimum = 0.0f, float maximum = 1.0f) const;
		[[nodiscard]] float sample3D(float x, float y, float z, float minimum = 0.0f, float maximum = 1.0f) const;
	};
}
