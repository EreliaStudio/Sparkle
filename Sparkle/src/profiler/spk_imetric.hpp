#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"

#include <limits>

namespace spk
{
	/**
	 * @class IMetric
	 * @brief Interface defining the structure for metric collection and report emission.
	 *
	 * This interface is designed to be implemented by classes that collect and analyze
	 * data to produce reports. Implementations of this interface must define the logic for
	 * determining when a report should be emitted and what the report will contain. This
	 * design allows for a flexible framework that can be extended to various types of metrics
	 * analysis and reporting, tailored to specific needs.
	 *
	 * Functions to implement:
	 * - shouldEmitReport(): Determines whether enough data has been collected to generate a report.
	 * - emitReport(): Generates and returns a report based on the collected data as a spk::JSON::Object.
	 *
	 * This interface is intended to be used in systems where monitoring and reporting on
	 * performance metrics, usage statistics, or other analytical data is required.
	 * 
	 * Usage example:
	 * @code
	 * class PerformanceMetric : public IMetric {
	 * public:
	 *     bool shouldEmitReport() override {
	 *         // Implementation logic here
	 *     }
	 *     spk::JSON::Object emitReport() override {
	 *         // Implementation logic here
	 *     }
	 * };
	 * @endcode
	 */
	class IMetric
	{
	private:
	public:
		virtual bool shouldEmitReport() = 0;
		virtual spk::JSON::Object emitReport() = 0;
	};

	/**
	 * @class IAnalyticalMetric<TType>
	 * @brief Template class for analytical metrics collection, extending IMetric.
	 *
	 * This class is a generic class designed to collect and analyze numerical
	 * data points of any type (e.g., int, float, double). It provides mechanisms to track the
	 * minimum, maximum, average values, and the total number of data points collected. This
	 * class is ideal for scenarios requiring detailed analysis of numerical data over time,
	 * such as performance monitoring, statistical analysis, or any application where metrics
	 * collection is necessary.
	 *
	 * The class implements the shouldEmitReport method from IMetric, indicating if data is
	 * available for reporting. It requires subclasses to implement the emitReport method,
	 * providing the flexibility to define custom report formats based on the collected data.
	 *
	 * Key functionalities include:
	 * - Dynamically updating metrics with new data points via saveValue().
	 * - Accessing the calculated minimum, maximum, and average values, and the total count of data points.
	 * - Leveraging template flexibility to work with various numerical types.
	 *
	 * Usage requires subclassing to implement the abstract emitReport() method, tailoring
	 * the data reporting to specific needs.
	 *
	 * @code
	 * class TemperatureMetric : public IAnalyticalMetric<float> {
	 * public:
	 *     spk::JSON::Object emitReport() override {
	 *         spk::JSON::Object report;
	 *         report.addAttribute("min").set(min());
	 *         report.addAttribute("max").set(max());
	 *         report.addAttribute("average").set(average());
	 *         return report;
	 *     }
	 *     void recordTemperature(float temperature) {
	 *         saveValue(temperature);
	 *     }
	 * };
	 * @endcode
	 */
	template <typename TType>
	class IAnalyticalMetric : public IMetric
	{
	private:
		TType _min = std::numeric_limits<TType>::max();
		TType _max = std::numeric_limits<TType>::min();
		float _average = 0;
		size_t _cardinal = 0;

	protected:
		void saveValue(const TType &p_newValue)
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

		const TType &min() const
		{
			return (_min);
		}

		const TType &max() const
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