#include "structure/math/spk_math_tester.hpp"

TEST(PositiveModuloTest, BasicTests)
{
	EXPECT_EQ(spk::positiveModulo(-12, 10), 8);
	EXPECT_EQ(spk::positiveModulo(-11, 10), 9);
	EXPECT_EQ(spk::positiveModulo(-10, 10), 0);
	EXPECT_EQ(spk::positiveModulo(-9, 10), 1);
	EXPECT_EQ(spk::positiveModulo(-8, 10), 2);
	EXPECT_EQ(spk::positiveModulo(-7, 10), 3);
	EXPECT_EQ(spk::positiveModulo(-6, 10), 4);
	EXPECT_EQ(spk::positiveModulo(-5, 10), 5);
	EXPECT_EQ(spk::positiveModulo(-4, 10), 6);
	EXPECT_EQ(spk::positiveModulo(-3, 10), 7);
	EXPECT_EQ(spk::positiveModulo(-2, 10), 8);
	EXPECT_EQ(spk::positiveModulo(-1, 10), 9);
	EXPECT_EQ(spk::positiveModulo(0, 10), 0);
	EXPECT_EQ(spk::positiveModulo(1, 10), 1);
	EXPECT_EQ(spk::positiveModulo(2, 10), 2);
	EXPECT_EQ(spk::positiveModulo(3, 10), 3);
	EXPECT_EQ(spk::positiveModulo(4, 10), 4);
	EXPECT_EQ(spk::positiveModulo(5, 10), 5);
	EXPECT_EQ(spk::positiveModulo(6, 10), 6);
	EXPECT_EQ(spk::positiveModulo(7, 10), 7);
	EXPECT_EQ(spk::positiveModulo(8, 10), 8);
	EXPECT_EQ(spk::positiveModulo(9, 10), 9);
	EXPECT_EQ(spk::positiveModulo(10, 10), 0);
	EXPECT_EQ(spk::positiveModulo(11, 10), 1);
	EXPECT_EQ(spk::positiveModulo(12, 10), 2);
}

TEST(PositiveModuloTest, ZeroModulo)
{
	EXPECT_THROW(spk::positiveModulo(5, 0), std::invalid_argument);
}

TEST(DegreeRadianConversionTest, DegreeToRadian)
{
	EXPECT_FLOAT_EQ(spk::degreeToRadian(0.0f), 0.0f);
	EXPECT_NEAR(spk::degreeToRadian(180.0f), static_cast<float>(M_PI), 1e-6f);
	EXPECT_NEAR(spk::degreeToRadian(-90.0f), static_cast<float>(-M_PI / 2.0), 1e-6f);
}

TEST(DegreeRadianConversionTest, RadianToDegree)
{
	EXPECT_FLOAT_EQ(spk::radianToDegree(0.0f), 0.0f);
	EXPECT_NEAR(spk::radianToDegree(static_cast<float>(M_PI)), 180.0f, 1e-5f);
	EXPECT_NEAR(spk::radianToDegree(static_cast<float>(-M_PI / 2.0)), -90.0f, 1e-5f);
}
