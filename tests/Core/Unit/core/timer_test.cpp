#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <type_traits>

#include "core/platform/controlled_timer.hpp"
#include "core/platform/system_timer.hpp"
#include "core/platform/timer.hpp"
#include "exception.hpp"

namespace
{
	using namespace std::chrono_literals;
}

TEST(TimerTest, AbstractInterfaceAndDurationValidation)
{
	static_assert(std::is_abstract_v<spk::Timer>);
	static_assert(std::is_base_of_v<spk::Timer, spk::SystemTimer>);
	static_assert(std::is_base_of_v<spk::Timer, spk::ControlledTimer>);
	EXPECT_THROW(spk::SystemTimer(0ms), spk::Exception);
	EXPECT_THROW(spk::ControlledTimer(-1ms), spk::Exception);
}

TEST(TimerTest, ControlledTimerAdvancesOnlyWhenStarted)
{
	spk::ControlledTimer timer(10ms);
	spk::Timer &interface = timer;
	EXPECT_EQ(interface.state(), spk::Timer::State::Off);
	timer.advance(3ms);
	EXPECT_EQ(interface.elapsedTime(), 0ms);

	interface.start();
	timer.advance(3ms);
	EXPECT_EQ(interface.state(), spk::Timer::State::Running);
	EXPECT_EQ(interface.elapsedTime(), 3ms);
	timer.advance(7ms);
	EXPECT_EQ(interface.state(), spk::Timer::State::TimedOut);
	EXPECT_EQ(interface.elapsedTime(), 10ms);
}

TEST(TimerTest, ControlledTimerStopResumeAndReset)
{
	spk::ControlledTimer timer(10ms);
	timer.start();
	timer.advance(4ms);
	EXPECT_EQ(timer.stop(), 4ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);

	timer.advance(9ms);
	EXPECT_EQ(timer.elapsedTime(), 4ms);
	timer.start();
	timer.advance(6ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);
	EXPECT_EQ(timer.stop(), 10ms);

	timer.reset();
	EXPECT_EQ(timer.elapsedTime(), 0ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	timer.start();
	timer.advance(2ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Running);
	EXPECT_THROW(timer.advance(-1ms), spk::Exception);
}

TEST(TimerTest, SystemTimerUsesRealSteadyClock)
{
	spk::SystemTimer timer(1ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	timer.start();
	std::this_thread::sleep_for(5ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);
	EXPECT_GE(timer.elapsedTime(), 1ms);
	EXPECT_GE(timer.stop(), 1ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);

	timer.reset();
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	EXPECT_EQ(timer.elapsedTime(), 0ms);
}

TEST(TimerTest, SharedStateMachineUsesVirtualElapsedPredicate)
{
	spk::ControlledTimer clock(10ms);
	spk::Timer &timer = clock;

	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	EXPECT_FALSE(timer.isElapsed());

	timer.start();
	clock.advance(9ms);
	EXPECT_FALSE(timer.isElapsed());
	EXPECT_EQ(timer.state(), spk::Timer::State::Running);

	clock.advance(1ms);
	EXPECT_TRUE(timer.isElapsed());
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	clock.advance(3ms);
	EXPECT_EQ(timer.elapsedTime(), 13ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	EXPECT_EQ(timer.stop(), 13ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	EXPECT_TRUE(timer.isElapsed());

	timer.start();
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	timer.reset();
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	EXPECT_EQ(timer.elapsedTime(), 0ms);
	EXPECT_FALSE(timer.isElapsed());
}

TEST(TimerTest, SharedStateMachineSupportsPolymorphicSystemTimer)
{
	spk::SystemTimer clock(1ms);
	spk::Timer &timer = clock;

	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	timer.start();
	std::this_thread::sleep_for(5ms);
	EXPECT_TRUE(timer.isElapsed());
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	const auto stopped = timer.stop();
	EXPECT_GE(stopped, 1ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
	std::this_thread::sleep_for(3ms);
	EXPECT_EQ(timer.elapsedTime(), stopped);

	timer.reset();
	EXPECT_EQ(timer.elapsedTime(), 0ms);
	EXPECT_EQ(timer.state(), spk::Timer::State::Off);
}
