#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "diagnostics/profiler.hpp"

namespace
{
	using namespace std::chrono_literals;

	TEST(ProfilerStatisticTest, TracksSummaryAndConfiguredAggregates)
	{
		spk::Profiler::Statistic<int> statistic;

		const auto &histogram = statistic.histogram(10);
		const auto &rolling = statistic.rollingStatistic(std::size_t{2});

		statistic.insert(5);
		statistic.insert(15);
		statistic.insert(25);

		EXPECT_EQ(statistic.nbSample(), 3);
		EXPECT_EQ(statistic.average(), 15);
		EXPECT_EQ(statistic.min(), 5);
		EXPECT_EQ(statistic.max(), 25);

		ASSERT_EQ(histogram.nbSample(), 3);
		EXPECT_EQ(histogram.buckets().at(0).count, 1);
		EXPECT_EQ(histogram.buckets().at(1).count, 1);
		EXPECT_EQ(histogram.buckets().at(2).count, 1);

		EXPECT_EQ(rolling.nbSample(), 2);
		EXPECT_EQ(rolling.average(), 20);
		EXPECT_EQ(rolling.min(), 15);
		EXPECT_EQ(rolling.max(), 25);
	}

	TEST(ProfilerStatisticTest, ConstAggregateLookupRejectsUnknownConfiguration)
	{
		spk::Profiler::Statistic<int> statistic;
		const auto &constStatistic = statistic;

		EXPECT_THROW(
			{ (void)constStatistic.histogram(10); },
			spk::Exception);
		EXPECT_THROW(
			{ (void)constStatistic.rollingStatistic(std::size_t{2}); },
			spk::Exception);
	}

	TEST(ProfilerTest, MeasurementIdentifiersAreStableAndMeasurementsAreReused)
	{
		spk::Profiler profiler;

		auto &first = profiler.gaugeMeasurement<int>("requests");
		auto &second = profiler.gaugeMeasurement<int>("requests");

		EXPECT_EQ(&first, &second);
		EXPECT_EQ(first.identifier(), "requests");
	}

	TEST(ProfilerTest, IdentifierCannotBeReusedByAnotherMeasurementType)
	{
		spk::Profiler profiler;
		(void)profiler.gaugeMeasurement<int>("shared");

		EXPECT_THROW(
			{ (void)profiler.sampleMeasurement<int>("shared"); },
			spk::Exception);
		EXPECT_THROW(
			{ (void)profiler.gaugeMeasurement<double>("shared"); },
			spk::Exception);
	}

	TEST(ProfilerTest, GaugeFeedsHistoricalStatistics)
	{
		spk::Profiler profiler;
		auto &gauge = profiler.gaugeMeasurement<int>("connections");

		gauge.insert(8);
		gauge.insert(2);
		gauge.insert(5);

		EXPECT_EQ(gauge.stats().nbSample(), 3);
		EXPECT_EQ(gauge.stats().average(), 5);
		EXPECT_EQ(gauge.stats().min(), 2);
		EXPECT_EQ(gauge.stats().max(), 8);
	}

	TEST(ProfilerTest, SampleMeasurementFeedsItsStatistics)
	{
		spk::Profiler profiler;
		auto &sample = profiler.sampleMeasurement<int>("queue-size");
		(void)sample.stats().histogram(10);
		(void)sample.stats().rollingStatistic(std::size_t{2});

		sample.add(3);
		sample.add(7);
		sample.add(11);

		EXPECT_EQ(sample.stats().average(), 7);
		EXPECT_EQ(sample.stats().min(), 3);
		EXPECT_EQ(sample.stats().max(), 11);
		EXPECT_EQ(sample.stats().histogram(10).nbSample(), 3);
		EXPECT_EQ(sample.stats().rollingStatistic(std::size_t{2}).average(), 9);
	}

	TEST(ProfilerTest, TimeMeasurementEnforcesStartStopLifecycle)
	{
		spk::Profiler profiler;
		auto &measurement = profiler.timeMeasurement("frame");

		EXPECT_FALSE(measurement.isRunning());
		EXPECT_THROW(measurement.stop(), spk::Exception);

		measurement.start();
		EXPECT_TRUE(measurement.isRunning());
		EXPECT_THROW(measurement.start(), spk::Exception);
		const auto beforeStop = measurement.stats().nbSample();
		measurement.stop();

		EXPECT_FALSE(measurement.isRunning());
		EXPECT_EQ(measurement.stats().nbSample(), beforeStop + 1);
		EXPECT_GE(measurement.stats().average(), spk::Profiler::TimeMeasurement::Duration::zero());
	}

	TEST(ProfilerTest, TimeMeasurementScopeStopsOnLeavingScope)
	{
		spk::Profiler profiler;
		auto &measurement = profiler.timeMeasurement("scoped-frame");

		{
			spk::Profiler::TimeMeasurement::Scope scope(measurement);
			EXPECT_TRUE(measurement.isRunning());
		}

		EXPECT_FALSE(measurement.isRunning());
		EXPECT_EQ(measurement.stats().nbSample(), 1);
	}

	TEST(ProfilerTest, RateMeasurementRefreshesAfterPeriodAndResetsCurrentValue)
	{
		spk::Profiler profiler;
		auto &rate = profiler.rateMeasurement<int>("events", 1ms);

		rate.increment(3);
		std::this_thread::sleep_for(5ms);
		rate.increment(2);

		EXPECT_EQ(rate.stats().nbSample(), 1);
		EXPECT_EQ(rate.stats().average(), 3);
		EXPECT_EQ(rate.value(), 2);
	}

	TEST(ProfilerTest, RateMeasurementValidatesPeriodAndKeepsConfiguration)
	{
		spk::Profiler profiler;

		EXPECT_THROW(
			{
				(void)profiler.rateMeasurement<int>(
					"invalid",
					spk::Chronometer::Duration::zero());
			},
			spk::Exception);

		(void)profiler.rateMeasurement<int>("events", 1ms);
		EXPECT_THROW(
			{ (void)profiler.rateMeasurement<int>("events", 2ms); },
			spk::Exception);
	}

	TEST(ProfilerTest, DefaultProfilerIsSingleton)
	{
		EXPECT_EQ(&spk::Profiler::defaultProfiler(), &spk::Profiler::defaultProfiler());
	}
}
