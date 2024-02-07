#pragma once

#include <random>
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "iostream/spk_iostream.hpp"
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
			intmax_t result =
				p_seed +
				static_cast<intmax_t>(p_x) * 374761393 +
				static_cast<intmax_t>(p_y) * 668265263 +
				static_cast<intmax_t>(p_z) * 982451653;
			result = (result ^ (result >> 13)) * 1274126177;
			return (result);
		}

		template <typename TOtherType>
		inline TGeneratedType _generate(const TOtherType& p_x, const TOtherType& p_y, const TOtherType& p_z) const
		{
			intmax_t value = _generateValue(_seed, p_x, p_y, p_z);

			if constexpr (std::is_floating_point<TGeneratedType>::value)
			{
				return (
					static_cast<TGeneratedType>(spk::positiveModulo(static_cast<intmax_t>(value), static_cast<intmax_t>(_range))) + //before ,
					static_cast<TGeneratedType>(spk::positiveModulo(static_cast<intmax_t>(_generateValue(_seed + 11, p_x, p_y, p_z)), _precisionModulo)) / _precisionModulo //After ,
				);
			}
			else
			{
				return (static_cast<TGeneratedType>(spk::positiveModulo(value, _range) + _min));
			}
		}

	public:
		PseudoRandomGenerator() 
			: _seed(std::random_device()())
		{

		}

		PseudoRandomGenerator(long long p_seed) 
			: _seed(p_seed)
		{

		}

		PseudoRandomGenerator(const spk::JSON::Object& p_object) 
			: _seed(p_object[L"Seed"].as<long>())
		{

		}

		void configureSeed(long long p_seed)
		{
			_seed = p_seed;
		}

		void configureRange(TGeneratedType p_min, TGeneratedType p_max)
		{
			if (p_min >= p_max)
				spk::throwException(L"Can set a range with min higher or equal to max");

			_min = p_min;
			_max = p_max;
			_range = _max - _min;
		}

		template<typename U = TGeneratedType, std::enable_if_t<std::is_floating_point<U>::value, int> = 0>
		void configurePrecision(uint32_t p_precision)
		{
			_precision = p_precision;
			_precisionModulo = std::pow(10, p_precision);
		}

		template <typename TOtherType>
		TGeneratedType sample(const spk::IVector3<TOtherType>& p_vector3) const {
			return _generate(
					static_cast<TGeneratedType>(p_vector3.x),
					static_cast<TGeneratedType>(p_vector3.y),
					static_cast<TGeneratedType>(p_vector3.z)
				);
		}

		template <typename TOtherType>
		TGeneratedType sample(const spk::IVector2<TOtherType>& p_vector2) const {
			return _generate(
					static_cast<TGeneratedType>(p_vector2.x),
					static_cast<TGeneratedType>(p_vector2.y),
					static_cast<TGeneratedType>(0)
				);
		}

		template <typename TOtherType>
		TGeneratedType sample(const TOtherType& p_x, const TOtherType& p_y = 0, const TOtherType& p_z = 0) const {
			return _generate(p_x, p_y, p_z);
		}
	};
}