#include <gtest/gtest.h>

#include <cstdlib>
#include <stdexcept>

#include "input/mouse.hpp"

TEST(MouseTest, StandardUsage)
{
	spk::Mouse mouse;
	for (const spk::InputState state : mouse.buttons)
	{
		EXPECT_EQ(state, spk::InputState::Up);
	}
	EXPECT_EQ(mouse.position.x, 0);
	EXPECT_EQ(mouse.position.y, 0);
	EXPECT_EQ(mouse.deltaPosition.x, 0);
	EXPECT_EQ(mouse.deltaPosition.y, 0);
	EXPECT_FLOAT_EQ(mouse.wheel, 0.0f);

	mouse[spk::Mouse::Left] = spk::InputState::Down;
	mouse[spk::Mouse::Middle] = spk::InputState::Down;
	mouse[spk::Mouse::Right] = spk::InputState::Up;
	mouse.position = {120, -45};
	mouse.deltaPosition = {7, -3};
	mouse.wheel = 1.5f;

	const spk::Mouse &constant = mouse;
	EXPECT_EQ(constant[spk::Mouse::Left], spk::InputState::Down);
	EXPECT_EQ(constant[spk::Mouse::Middle], spk::InputState::Down);
	EXPECT_EQ(constant[spk::Mouse::Right], spk::InputState::Up);
	EXPECT_EQ(constant.position.x, 120);
	EXPECT_EQ(constant.position.y, -45);
	EXPECT_EQ(constant.deltaPosition.x, 7);
	EXPECT_EQ(constant.deltaPosition.y, -3);
	EXPECT_FLOAT_EQ(constant.wheel, 1.5f);
}

TEST(MouseTest, EveryButtonSupportsMutableAndConstIndexing)
{
	spk::Mouse mouse;
	const spk::Mouse::Button buttons[] = {
		spk::Mouse::Right,
		spk::Mouse::Middle,
		spk::Mouse::Left,
	};

	for (const spk::Mouse::Button button : buttons)
	{
		mouse[button] = spk::InputState::Down;
		const spk::Mouse &constant = mouse;
		EXPECT_EQ(constant[button], spk::InputState::Down);
		mouse[button] = spk::InputState::Up;
	}
}

TEST(MouseTest, InvalidButtonThrows)
{
	// Isolate the potentially fatal MSVC debug-array assertion so an unmet
	// exception contract fails this test without terminating the whole suite.
	EXPECT_EXIT(
		{
			spk::Mouse mouse;
			const auto invalid = static_cast<spk::Mouse::Button>(spk::Mouse::NbButton);
			try
			{
				(void)mouse[invalid];
			}
			catch (const std::out_of_range &)
			{
				std::exit(0);
			}
			catch (...)
			{
				std::exit(2);
			}
			std::exit(1);
		},
		::testing::ExitedWithCode(0), "");

	EXPECT_EXIT(
		{
			const spk::Mouse mouse;
			const auto invalid = static_cast<spk::Mouse::Button>(spk::Mouse::NbButton);
			try
			{
				(void)mouse[invalid];
			}
			catch (const std::out_of_range &)
			{
				std::exit(0);
			}
			catch (...)
			{
				std::exit(2);
			}
			std::exit(1);
		},
		::testing::ExitedWithCode(0), "");
}
