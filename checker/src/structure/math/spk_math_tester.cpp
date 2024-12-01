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