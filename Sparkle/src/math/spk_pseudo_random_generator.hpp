#pragma once

#include <random>
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "spk_basic_functions.hpp"
#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk
{
	/**
	 * @class PseudoRandomGenerator
	 * 
	 * @tparam TGeneratedType The type of value to generate. Must be a arithmetic type (e.g., int, float, double).
	 * 
	 * @brief Template class for generating pseudo-random values of a specified numeric type within a given range.
	 *
	 * This class leverages a custom pseudo-random number generation algorithm to produce values for various use
	 * cases, including procedural generation, simulations, and any scenario requiring randomized numeric outputs.
	 * It supports integral and floating-point types as template parameters and allows customization of the seed,
	 * range, and precision (for floating-point types).
	 *
	 * The generator can produce values based on 2D or 3D coordinates, making it particularly useful for
	 * applications like noise generation, texture variation, or spatially dependent random events.
	 * The class ensures that results are reproducible by using a consistent seed and algorithmic approach.
	 *
	 * Usage example:
	 * @code
	 * spk::PseudoRandomGenerator<float> prg;
	 * prg.configureRange(0.0f, 1.0f);
	 * prg.configurePrecision(2);
	 * float randomValue = prg.sample(42, 13); // Generate a random float between 0.0 and 1.0
	 * @endcode
	 * 
	 * @note The precision configuration is applicable only to floating-point types and dictates the number of decimal places in generated values.
	 * 
	 * @see IVector2, IVector3
	 */
	template < typename TGeneratedType = int >
	class PseudoRandomGenerator
	{
		static_assert(std::is_arithmetic<TGeneratedType>::value, "TGeneratedType must be numeric inside a PseudoRandomGenerator.");

	private:
		intmax_t _seed;

		uint32_t _precision = 2;
		intmax_t _precisionModulo = 100;

		TGeneratedType _min = 0;
		TGeneratedType _max = 1;
		TGeneratedType _range = 1;

		template <typename TOtherType>
		inline intmax_t _generateValue(const intmax_t& p_seed, const TOtherType& p_x, const TOtherType& p_y, const TOtherType& p_z) const
		{
			intmax_t hash = p_seed;
			hash ^= static_cast<intmax_t>(p_x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= static_cast<intmax_t>(p_y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= static_cast<intmax_t>(p_z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			
			// Final mixing
			hash ^= hash >> 13;
			hash *= 1274126177;
			hash ^= hash >> 15;

			return hash;
		}

		template <typename TOtherType>
		inline TGeneratedType _generate(const TOtherType& p_x, const TOtherType& p_y, const TOtherType& p_z) const
		{
			intmax_t value = _generateValue(_seed, p_x, p_y, p_z);

			if constexpr (std::is_floating_point<TGeneratedType>::value)
			{
				TGeneratedType fixedPart = static_cast<TGeneratedType>((value % _range + _range) % _range) + _min;
				TGeneratedType floatingPart = ((_generateValue(_seed + 11, p_x, p_y, p_z) % _precisionModulo + _precisionModulo)% _precisionModulo) / _precisionModulo;

				return (fixedPart + floatingPart);
			}
			else
			{
				return (static_cast<TGeneratedType>(((value % _range + _range) % _range) + _min));
			}
		}

	public:
		/**
         * @brief Constructs a `PseudoRandomGenerator` with a randomly generated seed.
         * 
         * Initializes the generator with a seed value obtained from a `std::random_device`. This constructor is suitable for scenarios where a unique sequence of random values is desired across different program executions.
         */
		PseudoRandomGenerator() 
			: _seed(std::random_device()())
		{

		}

        /**
         * @brief Constructs a `PseudoRandomGenerator` with a specified seed.
         * 
         * Initializes the generator with the provided seed, allowing for reproducible sequences of random values. This is particularly useful for debugging or when a deterministic output is required.
         * 
         * @param p_seed The seed value to initialize the random number generator.
         */
		PseudoRandomGenerator(long long p_seed) 
			: _seed(p_seed)
		{

		}

		/**
         * @brief Configures the seed of the pseudo-random number generator.
         * 
         * Sets the seed value for the generator, affecting the sequence of generated values. Changing the seed allows for different sequences of numbers to be produced.
         * 
         * @param p_seed The new seed value.
         */
		void configureSeed(long long p_seed)
		{
			_seed = p_seed;
		}

        /**
         * @brief Configures the range of generated values.
         * 
         * Sets the minimum and maximum values for the generated numbers. This method defines the range of values that the generator can produce.
         * 
         * @param p_min The minimum value (included) in the range of generated numbers.
         * @param p_max The maximum value (excluded) in the range of generated numbers.
         */
		void configureRange(TGeneratedType p_min, TGeneratedType p_max)
		{
			if (p_min >= p_max)
				spk::throwException("Can set a range with min higher or equal to max");

			_min = p_min;
			_max = p_max;
			_range = _max - _min;
		}

        /**
         * @brief Configures the precision for floating-point number generation.
         * 
         * Sets the number of decimal places for generated floating-point values. This configuration is applicable only to floating-point types and affects the granularity of the generated numbers.
         * 
         * @param p_precision The number of decimal places for floating-point numbers.
         */
		template<typename U = TGeneratedType, std::enable_if_t<std::is_floating_point<U>::value, int> = 0>
		void configurePrecision(uint32_t p_precision)
		{
			_precision = p_precision;
			_precisionModulo = std::pow(10, p_precision);
		}

        /**
         * @brief Samples a pseudo-random value based on 3D coordinates.
         * 
         * Generates a random value of the specified type, influenced by the provided 3D coordinates. This method is useful for spatially varying random effects in 3D space.
         * 
         * @param p_vector3 A `spk::IVector3` representing the 3D coordinates.
         * @return A pseudo-random value of the specified type.
         */
		template <typename TOtherType>
		TGeneratedType sample(const spk::IVector3<TOtherType>& p_vector3) const {
			return _generate(
					static_cast<TGeneratedType>(p_vector3.x),
					static_cast<TGeneratedType>(p_vector3.y),
					static_cast<TGeneratedType>(p_vector3.z)
				);
		}

        /**
         * @brief Samples a pseudo-random value based on 2D coordinates.
         * 
         * Generates a random value of the specified type, influenced by the provided 2D coordinates. This method is useful for spatially varying random effects in 2D space.
         * 
         * @param p_vector2 A `spk::IVector2` representing the 2D coordinates.
         * @return A pseudo-random value of the specified type.
         */
		template <typename TOtherType>
		TGeneratedType sample(const spk::IVector2<TOtherType>& p_vector2) const {
			return _generate(
					static_cast<TGeneratedType>(p_vector2.x),
					static_cast<TGeneratedType>(p_vector2.y),
					static_cast<TGeneratedType>(0)
				);
		}
		
        /**
         * @brief Samples a pseudo-random value based on coordinates.
         * 
         * Generates a random value of the specified type, influenced by the provided coordinates. This generic method allows for 1D, 2D, or 3D spatial inputs.
         * 
         * @param p_x The x-coordinate.
         * @param p_y The y-coordinate (defaults to 0 if not provided).
         * @param p_z The z-coordinate (defaults to 0 if not provided).
         * @return A pseudo-random value of the specified type.
         */
		template <typename TOtherType>
		TGeneratedType sample(const TOtherType& p_x, const TOtherType& p_y = 0, const TOtherType& p_z = 0) const {
			return _generate(p_x, p_y, p_z);
		}
	};
}