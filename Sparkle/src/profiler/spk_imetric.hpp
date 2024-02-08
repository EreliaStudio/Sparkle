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
		/**
		 * @brief Determines if the current state warrants the generation of a report.
		 *
		 * Implementations should return true if the collected data meets certain conditions
		 * (e.g., a specific amount of data has been collected, a certain time has elapsed, etc.),
		 * indicating that a report should be generated.
		 *
		 * @return bool True if a report should be emitted based on the implementation's criteria,
		 *         false otherwise.
		 */
		virtual bool shouldEmitReport() = 0;
		
		/**
		 * @brief Generates a report based on the collected data.
		 *
		 * This method is called to generate a report, typically after shouldEmitReport() returns
		 * true. Implementations should construct and return a JSON object that encapsulates
		 * the relevant data and insights derived from the metric collection.
		 *
		 * @return spk::JSON::Object A JSON object containing the report data.
		 */
		virtual spk::JSON::Object emitReport() = 0;
	};

	/**
	 * @class IAnalyticalMetric
	 * 
	 * @tparam TType The type of data to manage. Must be a arithmetic type (e.g., int, float, double).
	 * 
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
		/**
		 * @brief Saves a new value into the metric, updating the minimum, maximum, average values, and the total number of data points collected.
		 * @param p_newValue The new data point to include in the metric analysis.
		 * 
		 * This method dynamically updates the metrics with each new data point, recalculating the
		 * minimum, maximum, and average values to reflect the most current state of the data. It is
		 * designed to be called whenever a new data point is available for collection.
		 */
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
		/**
		 * @brief Determines if enough data has been collected to generate a report.
		 * @return A boolean value indicating if a report should be emitted.
		 * 
		 * This method implements the shouldEmitReport method from the IMetric interface. It checks
		 * if any data points have been collected and returns true if there is data available for
		 * reporting, allowing for the dynamic determination of report generation based on the
		 * collected data.
		 */
		bool shouldEmitReport() override
		{
			return (_cardinal != 0);
		}

		/**
		 * @brief Abstract method to generate a report based on the collected data.
		 * @return A spk::JSON::Object containing the report data.
		 * 
		 * Subclasses must implement this method to define the specific contents and format of the
		 * report based on the collected metric data. This method provides the flexibility to
		 * tailor reports to specific analytical needs and data interpretations.
		 */
		virtual spk::JSON::Object emitReport() = 0;

		/**
		 * @brief Retrieves the minimum value recorded.
		 * @return The minimum value of the collected data points.
		 */
		const TType &min() const
		{
			return (_min);
		}

		/**
		 * @brief Retrieves the maximum value recorded.
		 * @return The maximum value of the collected data points.
		 */
		const TType &max() const
		{
			return (_max);
		}

		/**
		 * @brief Calculates the average value of the collected data points.
		 * @return The average value as a floating-point number.
		 * 
		 * The average is computed dynamically as new values are saved, providing an up-to-date
		 * calculation of the mean value of the collected data points.
		 */
		float average() const
		{
			return (_average);
		}

		/**
		 * @brief Retrieves the total number of data points collected.
		 * @return The total count of data points as a size_t.
		 */
		size_t cardinal() const
		{
			return (_cardinal);
		}
	};
}