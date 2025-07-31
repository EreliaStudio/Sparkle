#pragma once

#include <random>

namespace spk
{
	template <typename TGeneratedType = int>
	class RandomGenerator
	{
	private:
		using GeneratorType = typename std::conditional<std::is_same<TGeneratedType, double>::value || std::is_same<TGeneratedType, float>::value,
														std::uniform_real_distribution<TGeneratedType>,
														std::uniform_int_distribution<TGeneratedType>>::type;

		std::mt19937 _generator;
		GeneratorType _distribution;

	public:
		RandomGenerator() :
			_generator(std::random_device()()),
			_distribution()
		{
		}

		RandomGenerator(const unsigned int &p_seed) :
			_generator(p_seed),
			_distribution()
		{
		}

		RandomGenerator(const RandomGenerator &p_other) = delete;

		void configureSeed(const uint64_t &p_seed)
		{
			_generator = std::mt19937(p_seed);
		}

		void configureRange(const TGeneratedType &p_min, const TGeneratedType &p_max)
		{
			_distribution.param(typename GeneratorType::param_type(p_min, p_max));
		}

		constexpr TGeneratedType sample()
		{
			return _distribution(_generator);
		}

		constexpr TGeneratedType min() const
		{
			return _distribution.min();
		}
		constexpr TGeneratedType max() const
		{
			return _distribution.max();
		}
	};
}