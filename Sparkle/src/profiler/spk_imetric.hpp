#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"

#include <limits>

namespace spk
{
	class IMetric
	{
	private:

	public:
		virtual bool shouldEmitReport() = 0;
		virtual spk::JSON::Object emitReport() = 0;
	};

	template <typename TType>
	class IAnalyticalMetric : public IMetric
	{
	private:
		TType _min = std::numeric_limits<TType>::max();
		TType _max = std::numeric_limits<TType>::min();
		float _average = 0;
		size_t _cardinal = 0;

	protected:
		void saveValue(const TType& p_newValue)
		{
			if (_min > p_newValue)
				_min = p_newValue;
			if (_max < p_newValue)
				_max = p_newValue;

			_cardinal++;
			_average += (static_cast<float>(p_newValue) - static_cast<float>(_average)) / static_cast<float>(_cardinal);
		}
	public:
	
		bool shouldEmitReport() override
		{
			return (_cardinal != 0);
		}

		virtual spk::JSON::Object emitReport() = 0;

		const TType& min() const
		{
			return (_min);
		}

		const TType& max() const
		{
			return (_max);
		}

		float average() const
		{
			return (_average);
		}

		size_t cardinal() const
		{
			return (_cardinal);
		}
	};
}