#pragma once

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "container/histogram.hpp"
#include "container/polymorphic_container.hpp"
#include "container/rolling_statistic.hpp"
#include "core/platform/chronometer.hpp"
#include "core/platform/timer.hpp"

namespace spk
{
	class Profiler final
	{
	public:
		using Identifier = std::string;

		inline static constexpr const char *UpdateFrameMeasurement = "Update/frame";
		inline static constexpr const char *BuildRenderSnapshotMeasurement = "BuildRenderSnapshot/frame";
		inline static constexpr const char *RenderFrameMeasurement = "Render/frame";
		inline static constexpr const char *RenderCommandCountMeasurement = "Render/commands";
		inline static constexpr const char *RenderPassCommandCountMeasurementPrefix = "Render/commands/";
		inline static constexpr const char *RenderPassDuration = "/duration";

		class Measurement
		{
		private:
			Identifier _identifier;

		public:
			explicit Measurement(const Identifier &identifier);

			virtual ~Measurement();

			[[nodiscard]] const Identifier &identifier() const;
		};

		template <typename TType>
			requires requires(TType value, std::size_t count) {
				{ value + value } -> std::convertible_to<TType>;
				{ value * count } -> std::convertible_to<TType>;
				{ value / count } -> std::convertible_to<TType>;
				{ value < value } -> std::convertible_to<bool>;
			}
		class Statistic
		{
		public:
			using HistogramType = Histogram<TType>;
			using RollingStatisticType = RollingStatistic<TType>;

		private:
			TType _total{};
			TType _average{};
			TType _min{};
			TType _max{};
			std::size_t _nbSample = 0;

			std::map<TType, HistogramType> _histograms;
			std::map<TType, RollingStatisticType> _rollingStatistics;

		public:
			[[nodiscard]] const HistogramType &histogram(const TType &step)
			{
				auto [it, inserted] = _histograms.try_emplace(
					step,
					step);

				return it->second;
			}

			[[nodiscard]] const HistogramType &histogram(const TType &step) const
			{
				auto it = _histograms.find(step);

				if (it == _histograms.end())
				{
					throw spk::Exception("Unknown statistic histogram");
				}

				return it->second;
			}

			[[nodiscard]] const RollingStatisticType &rollingStatistic(std::size_t size)
			{
				auto [it, inserted] = _rollingStatistics.try_emplace(
					size,
					size);

				return it->second;
			}

			[[nodiscard]] const RollingStatisticType &rollingStatistic(const TType &size) const
			{
				auto it = _rollingStatistics.find(size);

				if (it == _rollingStatistics.end())
				{
					throw spk::Exception("Unknown statistic sample rollingStatistic");
				}

				return it->second;
			}

			void insert(const TType &value)
			{
				_total = _total + value;
				++_nbSample;

				if (_nbSample == 1)
				{
					_average = _min = _max = value;
				}
				else
				{
					_average = _total / _nbSample;
					_min = std::min(_min, value);
					_max = std::max(_max, value);
				}

				for (auto &[identifier, histogram] : _histograms)
				{
					histogram.insert(value);
				}

				for (auto &[size, window] : _rollingStatistics)
				{
					window.insert(value);
				}
			}

			[[nodiscard]] const TType &average() const
			{
				return _average;
			}

			[[nodiscard]] const TType &min() const
			{
				return _min;
			}

			[[nodiscard]] const TType &max() const
			{
				return _max;
			}

			[[nodiscard]] std::size_t nbSample() const
			{
				return _nbSample;
			}
		};

		class TimeMeasurement : public Measurement
		{
		public:
			using Duration = Chronometer::Duration;
			using Statistics = Statistic<Duration>;

			class Scope
			{
			private:
				TimeMeasurement &_measurement;

			public:
				explicit Scope(TimeMeasurement &measurement);

				Scope(const Scope &) = delete;
				Scope &operator=(const Scope &) = delete;
				Scope(Scope &&) = delete;
				Scope &operator=(Scope &&) = delete;

				~Scope() noexcept;
			};

		private:
			Statistics _stats;
			Chronometer _chronometer;
			bool _running = false;

		public:
			explicit TimeMeasurement(const Identifier &identifier);

			void start();
			void stop();

			[[nodiscard]] bool isRunning() const;

			[[nodiscard]] Statistics &stats();
			[[nodiscard]] const Statistics &stats() const;
		};

		template <typename TType>
		class SampleMeasurement : public Measurement
		{
		public:
			using Statistics = Statistic<TType>;

		private:
			Statistics _stats;

		public:
			explicit SampleMeasurement(const Identifier &identifier) :
				Measurement(identifier)
			{
			}

			void add(const TType &value)
			{
				_stats.insert(value);
			}

			[[nodiscard]] Statistics &stats()
			{
				return _stats;
			}

			[[nodiscard]] const Statistics &stats() const
			{
				return _stats;
			}
		};

		template <typename TType>
			requires std::is_arithmetic_v<TType>
		class GaugeMeasurement : public Measurement
		{
		public:
			using Statistics = Statistic<TType>;

		private:
			Statistics _stats;

		public:
			explicit GaugeMeasurement(const Identifier &identifier) :
				Measurement(identifier)
			{
			}

			void insert(const TType &value)
			{
				_stats.insert(value);
			}

			[[nodiscard]] Statistics &stats()
			{
				return _stats;
			}

			[[nodiscard]] const Statistics &stats() const
			{
				return _stats;
			}
		};

		template <typename TType>
			requires std::is_arithmetic_v<TType>
		class RateMeasurement : public Measurement
		{
		public:
			using Duration = Chronometer::Duration;
			using Statistics = Statistic<TType>;

		private:
			Statistics _stats;
			Timer _timer;
			TType _value{};

			void _refreshIfNecessary()
			{
				if (_timer.state() != Timer::State::TimedOut)
				{
					return;
				}

				_stats.insert(_value);
				_value = {};
				_timer.reset();
				_timer.start();
			}

		public:
			RateMeasurement(
				const Identifier &identifier,
				Duration refreshPeriod) :
				Measurement(identifier),
				_timer(refreshPeriod)
			{
				_timer.start();
			}

			void increment(const TType &delta = 1)
			{
				_refreshIfNecessary();
				_value += delta;
			}

			[[nodiscard]] const TType &value() const
			{
				return _value;
			}

			[[nodiscard]] const Duration &refreshPeriod() const
			{
				return _timer.duration();
			}

			[[nodiscard]] Statistics &stats()
			{
				return _stats;
			}

			[[nodiscard]] const Statistics &stats() const
			{
				return _stats;
			}
		};

	private:
		class MeasurementCollection final : private spk::PolymorphicContainer<Measurement>
		{
		public:
			template <typename TMeasurement>
				requires std::derived_from<TMeasurement, Measurement>
			[[nodiscard]] TMeasurement *find(const Identifier &identifier)
			{
				return this->template getElement<TMeasurement>(
					[&identifier](const TMeasurement *measurement) {
						return measurement->identifier() == identifier;
					});
			}

			[[nodiscard]] Measurement *find(const Identifier &identifier);

			template <typename TMeasurement, typename... TArgs>
				requires std::derived_from<TMeasurement, Measurement>
			[[nodiscard]] TMeasurement &emplace(TArgs &&...args)
			{
				return this->template emplaceElement<TMeasurement>(
					std::forward<TArgs>(args)...);
			}
		};

		mutable std::mutex _mutex;
		MeasurementCollection _measurements;

		template <typename TMeasurement, typename... TArgs>
			requires std::derived_from<TMeasurement, Measurement>
		[[nodiscard]] TMeasurement &_measurement(
			const Identifier &identifier,
			TArgs &&...args)
		{
			std::lock_guard lock(_mutex);

			if (TMeasurement *result =
					_measurements.template find<TMeasurement>(identifier);
				result != nullptr)
			{
				return *result;
			}

			if (_measurements.find(identifier) != nullptr)
			{
				throw spk::Exception(
					"Profiler identifier already used by another measurement type");
			}

			return _measurements.template emplace<TMeasurement>(
				identifier,
				std::forward<TArgs>(args)...);
		}

	public:
		Profiler() = default;

		[[nodiscard]] TimeMeasurement &timeMeasurement(
			const Identifier &identifier);

		template <typename TType = std::size_t>
			requires std::is_arithmetic_v<TType>
		[[nodiscard]] GaugeMeasurement<TType> &gaugeMeasurement(
			const Identifier &identifier)
		{
			return _measurement<GaugeMeasurement<TType>>(identifier);
		}

		template <typename TType = std::size_t>
			requires std::is_arithmetic_v<TType>
		[[nodiscard]] RateMeasurement<TType> &rateMeasurement(
			const Identifier &identifier,
			Chronometer::Duration refreshPeriod)
		{
			RateMeasurement<TType> &result =
				_measurement<RateMeasurement<TType>>(
					identifier,
					refreshPeriod);

			if (result.refreshPeriod() != refreshPeriod)
			{
				throw spk::Exception(
					"RateMeasurement already exists with another refresh period");
			}

			return result;
		}

		template <typename TType>
		[[nodiscard]] SampleMeasurement<TType> &sampleMeasurement(
			const Identifier &identifier)
		{
			return _measurement<SampleMeasurement<TType>>(identifier);
		}

		static Profiler &defaultProfiler();
	};
}
