#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <vector>

#include "ui/widget/toggle_switch.hpp"

TEST(ToggleSwitchTest, StateAnimationAndZeroDurationNotifyOnce)
{
	spk::ToggleSwitch toggle("Toggle");
	std::vector<bool> states;
	auto contract = toggle.subscribeToState([&](bool value) { states.push_back(value); });
	toggle.setChecked(true);
	toggle.setChecked(true);
	EXPECT_TRUE(toggle.isChecked());
	EXPECT_TRUE(toggle.isAnimating());
	EXPECT_EQ(states, (std::vector<bool>{true}));
	toggle.setAnimationDuration(spk::ToggleSwitch::Duration::zero());
	EXPECT_FALSE(toggle.isAnimating());
	EXPECT_FLOAT_EQ(toggle.visualRatio(), 1.0f);
	toggle.toggle();
	EXPECT_FALSE(toggle.isChecked());
	EXPECT_FLOAT_EQ(toggle.visualRatio(), 0.0f);
}

TEST(ToggleSwitchTest, OrientationPaddingThumbAndResourcesRoundTrip)
{
	spk::ToggleSwitch toggle("Toggle");
	const auto &style = spk::Widget::defaultStyle.get();
	ASSERT_NE(style.toggleSwitchOffBackground, nullptr);
	ASSERT_NE(style.toggleSwitchOnBackground, nullptr);
	toggle.setOrientation(spk::Orientation::Vertical);
	toggle.setPadding({3, 4});
	toggle.setThumbSize({18, 22});
	toggle.setUncheckedBackgroundTexture(style.toggleSwitchOffBackground.get());
	toggle.setCheckedBackgroundTexture(style.toggleSwitchOnBackground.get());
	toggle.setOutlineTexture(style.toggleSwitchOutline.get());
	toggle.setThumbTexture(style.toggleSwitchThumb.get());
	toggle.setOutlineCornerSize({5, 6});
	toggle.setThumbCornerSize({7, 8});
	EXPECT_EQ(toggle.orientation(), spk::Orientation::Vertical);
	EXPECT_EQ(toggle.padding(), spk::Vector2UInt(3, 4));
	EXPECT_EQ(toggle.thumbSize(), spk::Vector2UInt(18, 22));
	EXPECT_EQ(toggle.uncheckedBackgroundTexture(), style.toggleSwitchOffBackground.get());
	EXPECT_EQ(toggle.checkedBackgroundTexture(), style.toggleSwitchOnBackground.get());
	EXPECT_EQ(toggle.outlineCornerSize(), spk::Vector2Int(5, 6));
	EXPECT_EQ(toggle.thumbCornerSize(), spk::Vector2Int(7, 8));
}

TEST(ToggleSwitchTest, InvalidConfigurationThrowsAtomically)
{
	spk::ToggleSwitch toggle("Toggle");
	const auto duration = toggle.animationDuration();
	const auto *unchecked = toggle.uncheckedBackgroundTexture();
	EXPECT_THROW(toggle.setAnimationDuration(-std::chrono::milliseconds(1)), std::invalid_argument);
	EXPECT_THROW(toggle.setUncheckedBackgroundTexture(static_cast<const spk::Texture *>(nullptr)), std::invalid_argument);
	EXPECT_THROW(toggle.setCheckedBackgroundTexture(static_cast<const spk::Texture *>(nullptr)), std::invalid_argument);
	EXPECT_THROW(toggle.setOutlineTexture(nullptr), std::invalid_argument);
	EXPECT_THROW(toggle.setThumbTexture(nullptr), std::invalid_argument);
	const auto *notNineSlice = spk::Widget::defaultStyle.get().iconset.get();
	ASSERT_NE(notNineSlice, nullptr);
	EXPECT_NE(notNineSlice->nbSprite(), spk::Vector2UInt(3, 3));
	EXPECT_THROW(toggle.setUncheckedBackgroundTexture(notNineSlice), std::invalid_argument);
	EXPECT_THROW(toggle.setCheckedBackgroundTexture(notNineSlice), std::invalid_argument);
	EXPECT_EQ(toggle.animationDuration(), duration);
	EXPECT_EQ(toggle.uncheckedBackgroundTexture(), unchecked);
}
