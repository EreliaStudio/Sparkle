#include <gtest/gtest.h>

#include "ui/widget/numeric_spin_box.hpp"
#include "ui/widget/spin_box.hpp"
#include <limits>
#include <vector>

namespace
{
	template <typename T>
	class SpinBoxTypedTest : public testing::Test
	{
	};
	using Numbers = testing::Types<int, unsigned int, float, double>;
	TYPED_TEST_SUITE(SpinBoxTypedTest, Numbers);
}

TYPED_TEST(SpinBoxTypedTest, LimitsStepsClampingNotificationsAndTextStaySynchronized)
{
	spk::SpinBox<TypeParam> box("Spin");
	int editions = 0;
	auto contract = box.subscribeToEdition([&](auto value) {
		++editions;
		EXPECT_EQ(value, box.value());
	});
	EXPECT_FALSE(box.valueEdit().isEditEnabled());
	box.setMinimum(2);
	box.setMaximum(8);
	box.setStep(2);
	EXPECT_EQ(box.value(), 2);
	EXPECT_EQ(box.step(), 2);
	box.increase();
	EXPECT_EQ(box.value(), 4);
	box.decrease();
	EXPECT_EQ(box.value(), 2);
	box.setValue(100);
	EXPECT_EQ(box.value(), 8);
	const int before = editions;
	box.increase();
	box.setValue(8);
	EXPECT_EQ(editions, before);
	box.setStep(0);
	box.decrease();
	EXPECT_EQ(editions, before);
	EXPECT_EQ(box.valueEdit().textAsUTF8(), std::to_string(box.value()));
	box.setMinimum(8);
	box.setMaximum(8);
	box.setValue(0);
	EXPECT_EQ(box.value(), 8);
	box.removeMinimum();
	EXPECT_FALSE(box.minimum());
	box.removeMaximum();
	EXPECT_FALSE(box.maximum());
	box.setValue(10);
	EXPECT_EQ(box.value(), 10);
	box.setMinimum(0);
	box.setMaximum(20);
	box.removeLimits();
	EXPECT_FALSE(box.minimum());
	EXPECT_FALSE(box.maximum());
	const auto &constant = box;
	EXPECT_EQ(&constant.downButton(), &box.downButton());
	EXPECT_EQ(&constant.upButton(), &box.upButton());
	EXPECT_EQ(&constant.valueEdit(), &box.valueEdit());
}

TYPED_TEST(SpinBoxTypedTest, NumericEditingValidUndefinedAndInvalidInputsDoNotRecurse)
{
	spk::NumericSpinBox<TypeParam> box("Number");
	int editions = 0;
	auto contract = box.subscribeToEdition([&](auto value) {
		++editions;
		EXPECT_EQ(box.value(), value);
	});
	box.valueEdit().setText("+12");
	EXPECT_EQ(box.value(), 12);
	EXPECT_EQ(editions, 1);
	box.valueEdit().setText("012");
	EXPECT_EQ(editions, 1);
	box.setStep(2);
	box.increase();
	EXPECT_EQ(box.value(), 14);
	EXPECT_EQ(editions, 2);
	EXPECT_EQ(box.valueEdit().textAsUTF8(), std::to_string(box.value()));
	box.decrease();
	EXPECT_EQ(box.value(), 12);
	EXPECT_EQ(editions, 3);
	for (const auto text : {"", "+"})
	{
		box.valueEdit().setText(text);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Undefined);
		EXPECT_EQ(box.value(), 12);
	}
	for (const auto &text : std::vector<std::string>{" 12", "12 ", "12x", "++1", "0x10", "\xc3\xa9", std::string(400, '9')})
	{
		box.valueEdit().setText(text);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Invalid) << text;
		EXPECT_EQ(box.value(), 12);
	}
	EXPECT_EQ(editions, 3);
	if constexpr (std::is_unsigned_v<TypeParam>)
	{
		box.valueEdit().setText("-1");
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Invalid);
	}
	else
	{
		box.valueEdit().setText("-");
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Undefined);
		box.valueEdit().setText("-12");
		EXPECT_EQ(box.value(), -12);
		box.setStep(-2);
		box.increase();
		EXPECT_EQ(box.value(), -14);
	}
}

TEST(NumericSpinBoxTest, FloatingGrammarIncludesDecimalsExponentsAndIntermediateTokens)
{
	spk::NumericSpinBox<double> box("Float");
	for (const auto &[text, value] : {std::pair{".5", 0.5}, {"-.5", -0.5}, {"1.", 1.0}, {"1e2", 100.0}, {"+1.25e-2", 0.0125}})
	{
		box.valueEdit().setText(text);
		EXPECT_DOUBLE_EQ(box.value(), value);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Valid);
	}
	for (const auto text : {".", "-.", "+."})
	{
		box.valueEdit().setText(text);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Undefined);
	}
	for (const auto text : {"1e", "1e+", "1,2", "1.2.3", "1e9999"})
	{
		box.valueEdit().setText(text);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Invalid);
	}
}

TEST(NumericSpinBoxTest, FloatingFromCharsGrammarAcceptsNonFiniteTokens)
{
	spk::NumericSpinBox<double> box("Float");
	box.valueEdit().setText("inf");
	EXPECT_TRUE(std::isinf(box.value()));
	EXPECT_GT(box.value(), 0);
	box.valueEdit().setText("-inf");
	EXPECT_TRUE(std::isinf(box.value()));
	EXPECT_LT(box.value(), 0);
	box.valueEdit().setText("nan");
	EXPECT_TRUE(std::isnan(box.value()));
	EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Valid);
}

TEST(NumericControlsTest, NarrowGeometrySpriteChangesStylesAndButtonClicks)
{
	spk::SpinBox<int> spin("Spin");
	spk::NumericSpinBox<int> numeric("Numeric");
	for (const unsigned width : {0u, 1u, 10u, 200u})
	{
		spin.setGeometry({.anchor = {0, 0}, .size = {width, 30}});
		numeric.setGeometry({.anchor = {0, 0}, .size = {width, 30}});
		EXPECT_EQ(spin.downButton().geometry().width + spin.valueEdit().geometry().width + spin.upButton().geometry().width, width);
		EXPECT_EQ(numeric.lowerButton().geometry().width + numeric.valueEdit().geometry().width + numeric.raiseButton().geometry().width, width);
	}
	spin.setButtonSpriteIDs(1, 2);
	numeric.setButtonSpriteIDs(2, 3);
	spin.applyStyle(spk::Widget::defaultStyle);
	numeric.applyStyle(spk::Widget::defaultStyle);
	EXPECT_EQ(spin.valueEdit().font(), spk::Widget::defaultStyle->font.get());
	EXPECT_EQ(numeric.valueEdit().font(), spk::Widget::defaultStyle->font.get());
	for (auto *button : {&spin.upButton(), &numeric.raiseButton()})
	{
		spk::Mouse mouse;
		mouse.position = button->viewRegion().viewport.anchor + spk::Vector2Int{5, 5};
		spk::MouseButtonPressedRecord press{};
		press.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressed(press, mouse);
		button->dispatch(pressed);
		spk::MouseButtonReleasedRecord release{};
		release.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent released(release, mouse);
		button->dispatch(released);
	}
	EXPECT_EQ(spin.value(), 1);
	EXPECT_EQ(numeric.value(), 1);
}

// Unsigned extrema provide a defined arithmetic oracle; signed overflow must not be executed.
TEST(SpinBoxTest, DISABLED_ArithmeticAtUnsignedExtremaSaturatesBeforeApplyingLimits)
{
	spk::SpinBox<unsigned> box("Spin");
	box.setMinimum(0);
	box.setMaximum(std::numeric_limits<unsigned>::max());
	box.decrease();
	EXPECT_EQ(box.value(), 0u);
	box.setValue(std::numeric_limits<unsigned>::max());
	box.increase();
	EXPECT_EQ(box.value(), std::numeric_limits<unsigned>::max());
}

TEST(SpinBoxTest, InvertedLimitsUseMaximumAfterMinimumAndCanBeRemovedIndependently)
{
	spk::SpinBox<int> box("Spin");
	box.setMinimum(10);
	box.setMaximum(5);
	box.setValue(7);
	EXPECT_EQ(box.minimum(), 10);
	EXPECT_EQ(box.maximum(), 5);
	EXPECT_EQ(box.value(), 5);
	box.removeMaximum();
	box.setValue(7);
	EXPECT_EQ(box.value(), 10);
	box.removeMinimum();
	box.setValue(-10);
	EXPECT_EQ(box.value(), -10);
	box.setStep(-2);
	box.increase();
	EXPECT_EQ(box.value(), -12);
	box.decrease();
	EXPECT_EQ(box.value(), -10);
	EXPECT_THROW(box.setIconset(nullptr), std::invalid_argument);
}

// int8_t arithmetic is promoted to int, so these boundary checks do not execute signed overflow.
TEST(SpinBoxTest, DISABLED_SignedExtremaSaturateBeforeNarrowing)
{
	spk::SpinBox<std::int8_t> box("Signed");
	box.setMinimum(-128);
	box.setMaximum(127);
	box.setValue(127);
	box.increase();
	EXPECT_EQ(box.value(), 127);
	box.setValue(-128);
	box.decrease();
	EXPECT_EQ(box.value(), -128);
}

TYPED_TEST(SpinBoxTypedTest, NumericGrammarAcceptsRepresentableExtrema)
{
	spk::NumericSpinBox<TypeParam> box("Extrema");
	for (TypeParam value : {std::numeric_limits<TypeParam>::lowest(), std::numeric_limits<TypeParam>::max()})
	{
		const auto text = std::to_string(value);
		box.valueEdit().setText(text);
		EXPECT_EQ(box.valueEdit().validationState(), spk::TextEdit::ValidationState::Valid);
		EXPECT_EQ(box.value(), value);
	}
}
