#include "structure/math/spk_vector3_tester.hpp"

TEST_F(Vector3Test, DefaultConstructor)
{
	spk::IVector3<int> vecInt;
	EXPECT_EQ(vecInt.x, 0) << "Default constructor for int should initialize x to 0";
	EXPECT_EQ(vecInt.y, 0) << "Default constructor for int should initialize y to 0";
	EXPECT_EQ(vecInt.z, 0) << "Default constructor for int should initialize z to 0";

	spk::IVector3<float> vecFloat;
	EXPECT_EQ(vecFloat.x, 0.0f) << "Default constructor for float should initialize x to 0.0f";
	EXPECT_EQ(vecFloat.y, 0.0f) << "Default constructor for float should initialize y to 0.0f";
	EXPECT_EQ(vecFloat.z, 0.0f) << "Default constructor for float should initialize z to 0.0f";

	spk::IVector3<double> vecDouble;
	EXPECT_EQ(vecDouble.x, 0.0) << "Default constructor for double should initialize x to 0.0";
	EXPECT_EQ(vecDouble.y, 0.0) << "Default constructor for double should initialize y to 0.0";
	EXPECT_EQ(vecDouble.z, 0.0) << "Default constructor for double should initialize z to 0.0";

	spk::IVector3<size_t> vecSizeT;
	EXPECT_EQ(vecSizeT.x, 0) << "Default constructor for size_t should initialize x to 0";
	EXPECT_EQ(vecSizeT.y, 0) << "Default constructor for size_t should initialize y to 0";
	EXPECT_EQ(vecSizeT.z, 0) << "Default constructor for size_t should initialize z to 0";
}

TEST_F(Vector3Test, ScalarConstructor)
{
	spk::IVector3<int> vecInt(5);
	EXPECT_EQ(vecInt.x, 5) << "Scalar constructor for int should initialize x to 5";
	EXPECT_EQ(vecInt.y, 5) << "Scalar constructor for int should initialize y to 5";
	EXPECT_EQ(vecInt.z, 5) << "Scalar constructor for int should initialize z to 5";

	spk::IVector3<float> vecFloat(5.5f);
	EXPECT_FLOAT_EQ(vecFloat.x, 5.5f) << "Scalar constructor for float should initialize x to 5.5f";
	EXPECT_FLOAT_EQ(vecFloat.y, 5.5f) << "Scalar constructor for float should initialize y to 5.5f";
	EXPECT_FLOAT_EQ(vecFloat.z, 5.5f) << "Scalar constructor for float should initialize z to 5.5f";

	spk::IVector3<double> vecDouble(5.5);
	EXPECT_DOUBLE_EQ(vecDouble.x, 5.5) << "Scalar constructor for double should initialize x to 5.5";
	EXPECT_DOUBLE_EQ(vecDouble.y, 5.5) << "Scalar constructor for double should initialize y to 5.5";
	EXPECT_DOUBLE_EQ(vecDouble.z, 5.5) << "Scalar constructor for double should initialize z to 5.5";

	spk::IVector3<size_t> vecSizeT(5);
	EXPECT_EQ(vecSizeT.x, 5) << "Scalar constructor for size_t should initialize x to 5";
	EXPECT_EQ(vecSizeT.y, 5) << "Scalar constructor for size_t should initialize y to 5";
	EXPECT_EQ(vecSizeT.z, 5) << "Scalar constructor for size_t should initialize z to 5";
}

TEST_F(Vector3Test, ThreeValueConstructor)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	EXPECT_EQ(vecInt.x, 3) << "Three-value constructor for int should initialize x to 3";
	EXPECT_EQ(vecInt.y, 4) << "Three-value constructor for int should initialize y to 4";
	EXPECT_EQ(vecInt.z, 5) << "Three-value constructor for int should initialize z to 5";

	spk::IVector3<float> vecFloat(3.5f, 4.5f, 5.5f);
	EXPECT_FLOAT_EQ(vecFloat.x, 3.5f) << "Three-value constructor for float should initialize x to 3.5f";
	EXPECT_FLOAT_EQ(vecFloat.y, 4.5f) << "Three-value constructor for float should initialize y to 4.5f";
	EXPECT_FLOAT_EQ(vecFloat.z, 5.5f) << "Three-value constructor for float should initialize z to 5.5f";

	spk::IVector3<double> vecDouble(3.5, 4.5, 5.5);
	EXPECT_DOUBLE_EQ(vecDouble.x, 3.5) << "Three-value constructor for double should initialize x to 3.5";
	EXPECT_DOUBLE_EQ(vecDouble.y, 4.5) << "Three-value constructor for double should initialize y to 4.5";
	EXPECT_DOUBLE_EQ(vecDouble.z, 5.5) << "Three-value constructor for double should initialize z to 5.5";

	spk::IVector3<size_t> vecSizeT(3, 4, 5);
	EXPECT_EQ(vecSizeT.x, 3) << "Three-value constructor for size_t should initialize x to 3";
	EXPECT_EQ(vecSizeT.y, 4) << "Three-value constructor for size_t should initialize y to 4";
	EXPECT_EQ(vecSizeT.z, 5) << "Three-value constructor for size_t should initialize z to 5";
}

TEST_F(Vector3Test, CopyConstructor)
{
	spk::IVector3<int> vecInt1(3, 4, 5);
	spk::IVector3<int> vecInt2(vecInt1);
	EXPECT_EQ(vecInt2.x, 3) << "Copy constructor for int should copy x value";
	EXPECT_EQ(vecInt2.y, 4) << "Copy constructor for int should copy y value";
	EXPECT_EQ(vecInt2.z, 5) << "Copy constructor for int should copy z value";

	spk::IVector3<float> vecFloat1(3.5f, 4.5f, 5.5f);
	spk::IVector3<float> vecFloat2(vecFloat1);
	EXPECT_FLOAT_EQ(vecFloat2.x, 3.5f) << "Copy constructor for float should copy x value";
	EXPECT_FLOAT_EQ(vecFloat2.y, 4.5f) << "Copy constructor for float should copy y value";
	EXPECT_FLOAT_EQ(vecFloat2.z, 5.5f) << "Copy constructor for float should copy z value";

	spk::IVector3<double> vecDouble1(3.5, 4.5, 5.5);
	spk::IVector3<double> vecDouble2(vecDouble1);
	EXPECT_DOUBLE_EQ(vecDouble2.x, 3.5) << "Copy constructor for double should copy x value";
	EXPECT_DOUBLE_EQ(vecDouble2.y, 4.5) << "Copy constructor for double should copy y value";
	EXPECT_DOUBLE_EQ(vecDouble2.z, 5.5) << "Copy constructor for double should copy z value";

	spk::IVector3<size_t> vecSizeT1(3, 4, 5);
	spk::IVector3<size_t> vecSizeT2(vecSizeT1);
	EXPECT_EQ(vecSizeT2.x, 3) << "Copy constructor for size_t should copy x value";
	EXPECT_EQ(vecSizeT2.y, 4) << "Copy constructor for size_t should copy y value";
	EXPECT_EQ(vecSizeT2.z, 5) << "Copy constructor for size_t should copy z value";
}

TEST_F(Vector3Test, EqualityOperator)
{
	spk::IVector3<int> vecInt1(3, 4, 5);
	spk::IVector3<int> vecInt2(3, 4, 5);
	spk::IVector3<int> vecInt3(6, 4, 5);
	EXPECT_EQ(vecInt1, vecInt2) << "Equality operator should return true for equal int vectors";
	EXPECT_NE(vecInt1, vecInt3) << "Equality operator should return false for different int vectors";
	vecInt3 = spk::IVector3<int>(3, 5, 5);
	EXPECT_NE(vecInt1, vecInt3) << "Equality operator should return false for different int vectors";
	vecInt3 = spk::IVector3<int>(3, 4, 6);
	EXPECT_NE(vecInt1, vecInt3) << "Equality operator should return false for different int vectors";

	spk::IVector3<float> vecFloat1(3.0f, 4.0f, 5.0f);
	spk::IVector3<float> vecFloat2(3.0f, 4.0f, 5.0f);
	spk::IVector3<float> vecFloat3(6.0f, 7.0f, 8.0f);
	EXPECT_EQ(vecFloat1, vecFloat2) << "Equality operator should return true for equal float vectors";
	EXPECT_NE(vecFloat1, vecFloat3) << "Equality operator should return false for different float vectors";
	EXPECT_NE(vecFloat1, vecFloat3) << "Equality operator should return false for different int vectors";
	vecFloat3 = spk::IVector3<float>(3, 5, 5);
	EXPECT_NE(vecFloat1, vecFloat3) << "Equality operator should return false for different int vectors";
	vecFloat3 = spk::IVector3<float>(3, 4, 6);
	EXPECT_NE(vecFloat1, vecFloat3) << "Equality operator should return false for different int vectors";

	spk::IVector3<double> vecDouble1(3.0, 4.0, 5.0);
	spk::IVector3<double> vecDouble2(3.0, 4.0, 5.0);
	spk::IVector3<double> vecDouble3(6.0, 7.0, 8.0);
	EXPECT_EQ(vecDouble1, vecDouble2) << "Equality operator should return true for equal double vectors";
	EXPECT_NE(vecDouble1, vecDouble3) << "Equality operator should return false for different double vectors";
	EXPECT_NE(vecDouble1, vecDouble3) << "Equality operator should return false for different int vectors";
	vecDouble3 = spk::IVector3<double>(3, 5, 5);
	EXPECT_NE(vecDouble1, vecDouble3) << "Equality operator should return false for different int vectors";
	vecDouble3 = spk::IVector3<double>(3, 4, 6);
	EXPECT_NE(vecDouble1, vecDouble3) << "Equality operator should return false for different int vectors";

	spk::IVector3<size_t> vecSizeT1(3, 4, 5);
	spk::IVector3<size_t> vecSizeT2(3, 4, 5);
	spk::IVector3<size_t> vecSizeT3(6, 7, 8);
	EXPECT_EQ(vecSizeT1, vecSizeT2) << "Equality operator should return true for equal size_t vectors";
	EXPECT_NE(vecSizeT1, vecSizeT3) << "Equality operator should return false for different size_t vectors";
	EXPECT_NE(vecSizeT1, vecSizeT3) << "Equality operator should return false for different int vectors";
	vecSizeT3 = spk::IVector3<size_t>(3, 5, 5);
	EXPECT_NE(vecSizeT1, vecSizeT3) << "Equality operator should return false for different int vectors";
	vecSizeT3 = spk::IVector3<size_t>(3, 4, 6);
	EXPECT_NE(vecSizeT1, vecSizeT3) << "Equality operator should return false for different int vectors";

	spk::IVector3<int> vecInt4(3, 4, 5);
	spk::IVector3<float> vecFloat4(3.5f, 4.0f, 5.0f);

	EXPECT_TRUE(vecInt4 == vecFloat4) << "Upon casting float to int, vecFloat should be equal as 3.5 should be concidered as 3";
	EXPECT_FALSE(vecFloat4 == vecInt4) << "Upon casting int to float, vecInt should be equal as 3 should be concidered as 3.0 and therefor must be different";
}

TEST_F(Vector3Test, MixedTypeEqualityOperator)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	spk::IVector3<float> vecFloat(3.0f, 4.0f, 5.0f);
	spk::IVector3<double> vecDouble(3.0, 4.0, 5.0);
	spk::IVector3<size_t> vecSizeT(3, 4, 5);

	EXPECT_EQ(vecInt, vecFloat) << "Mixed-type equality operator should return true for equivalent int and float vectors";
	EXPECT_EQ(vecInt, vecDouble) << "Mixed-type equality operator should return true for equivalent int and double vectors";
	EXPECT_EQ(vecInt, vecSizeT) << "Mixed-type equality operator should return true for equivalent int and size_t vectors";
	EXPECT_EQ(vecFloat, vecDouble) << "Mixed-type equality operator should return true for equivalent float and double vectors";
	EXPECT_EQ(vecFloat, vecSizeT) << "Mixed-type equality operator should return true for equivalent float and size_t vectors";
	EXPECT_EQ(vecDouble, vecSizeT) << "Mixed-type equality operator should return true for equivalent double and size_t vectors";

	spk::IVector3<int> vecIntDifferent(6, 7, 8);
	spk::IVector3<float> vecFloatDifferent(6.0f, 7.0f, 8.0f);
	spk::IVector3<double> vecDoubleDifferent(6.0, 7.0, 8.0);
	spk::IVector3<size_t> vecSizeTDifferent(6, 7, 8);

	EXPECT_NE(vecInt, vecIntDifferent) << "Mixed-type equality operator should return false for different int and int vectors";
	EXPECT_NE(vecInt, vecFloatDifferent) << "Mixed-type equality operator should return false for different int and float vectors";
	EXPECT_NE(vecInt, vecDoubleDifferent) << "Mixed-type equality operator should return false for different int and double vectors";
	EXPECT_NE(vecInt, vecSizeTDifferent) << "Mixed-type equality operator should return false for different int and size_t vectors";
	EXPECT_NE(vecFloat, vecFloatDifferent) << "Mixed-type equality operator should return false for different float and float vectors";
	EXPECT_NE(vecFloat, vecDoubleDifferent) << "Mixed-type equality operator should return false for different float and double vectors";
	EXPECT_NE(vecFloat, vecSizeTDifferent) << "Mixed-type equality operator should return false for different float and size_t vectors";
	EXPECT_NE(vecDouble, vecDoubleDifferent) << "Mixed-type equality operator should return false for different double and double vectors";
	EXPECT_NE(vecDouble, vecSizeTDifferent) << "Mixed-type equality operator should return false for different double and size_t vectors";
	EXPECT_NE(vecSizeT, vecSizeTDifferent) << "Mixed-type equality operator should return false for different size_t and size_t vectors";
}

TEST_F(Vector3Test, ComparatorOperators)
{
	spk::IVector3<int> value(5, 6, 7);
	spk::IVector3<int> xEqualYEqualZEqual(5, 6, 7);
	spk::IVector3<int> xEqualYEqualZBigger(5, 6, 8);
	spk::IVector3<int> xEqualYEqualZLower(5, 6, 6);
	spk::IVector3<int> xEqualYBiggerZEqual(5, 7, 7);
	spk::IVector3<int> xEqualYLowerZEqual(5, 5, 7);
	spk::IVector3<int> xBiggerYEqualZEqual(6, 6, 7);
	spk::IVector3<int> xLowerYEqualZEqual(4, 6, 7);

	EXPECT_FALSE(value < xEqualYEqualZEqual) << "Comparing with X Equal, Y Equal, and Z Equal with operator < should return false";
	EXPECT_TRUE(value < xEqualYEqualZBigger) << "Comparing with X Equal, Y Equal, and Z Bigger with operator < should return true";
	EXPECT_FALSE(value < xEqualYEqualZLower) << "Comparing with X Equal, Y Equal, and Z Lower with operator < should return false";
	EXPECT_TRUE(value < xEqualYBiggerZEqual) << "Comparing with X Equal, Y Bigger, and Z Equal with operator < should return true";
	EXPECT_FALSE(value < xEqualYLowerZEqual) << "Comparing with X Equal, Y Lower, and Z Equal with operator < should return false";
	EXPECT_TRUE(value < xBiggerYEqualZEqual) << "Comparing with X Bigger, Y Equal, and Z Equal with operator < should return true";
	EXPECT_FALSE(value < xLowerYEqualZEqual) << "Comparing with X Lower, Y Equal, and Z Equal with operator < should return false";

	EXPECT_FALSE(value > xEqualYEqualZEqual) << "Comparing with X Equal, Y Equal, and Z Equal with operator > should return false";
	EXPECT_FALSE(value > xEqualYEqualZBigger) << "Comparing with X Equal, Y Equal, and Z Bigger with operator > should return false";
	EXPECT_TRUE(value > xEqualYEqualZLower) << "Comparing with X Equal, Y Equal, and Z Lower with operator > should return true";
	EXPECT_FALSE(value > xEqualYBiggerZEqual) << "Comparing with X Equal, Y Bigger, and Z Equal with operator > should return false";
	EXPECT_TRUE(value > xEqualYLowerZEqual) << "Comparing with X Equal, Y Lower, and Z Equal with operator > should return true";
	EXPECT_FALSE(value > xBiggerYEqualZEqual) << "Comparing with X Bigger, Y Equal, and Z Equal with operator > should return false";
	EXPECT_TRUE(value > xLowerYEqualZEqual) << "Comparing with X Lower, Y Equal, and Z Equal with operator > should return true";

	EXPECT_TRUE(value <= xEqualYEqualZEqual) << "Comparing with X Equal, Y Equal, and Z Equal with operator <= should return true";
	EXPECT_TRUE(value <= xEqualYEqualZBigger) << "Comparing with X Equal, Y Equal, and Z Bigger with operator <= should return true";
	EXPECT_FALSE(value <= xEqualYEqualZLower) << "Comparing with X Equal, Y Equal, and Z Lower with operator <= should return false";
	EXPECT_TRUE(value <= xEqualYBiggerZEqual) << "Comparing with X Equal, Y Bigger, and Z Equal with operator <= should return true";
	EXPECT_FALSE(value <= xEqualYLowerZEqual) << "Comparing with X Equal, Y Lower, and Z Equal with operator <= should return false";
	EXPECT_TRUE(value <= xBiggerYEqualZEqual) << "Comparing with X Bigger, Y Equal, and Z Equal with operator <= should return true";
	EXPECT_FALSE(value <= xLowerYEqualZEqual) << "Comparing with X Lower, Y Equal, and Z Equal with operator <= should return false";

	EXPECT_TRUE(value >= xEqualYEqualZEqual) << "Comparing with X Equal, Y Equal, and Z Equal with operator >= should return true";
	EXPECT_FALSE(value >= xEqualYEqualZBigger) << "Comparing with X Equal, Y Equal, and Z Bigger with operator >= should return false";
	EXPECT_TRUE(value >= xEqualYEqualZLower) << "Comparing with X Equal, Y Equal, and Z Lower with operator >= should return true";
	EXPECT_FALSE(value >= xEqualYBiggerZEqual) << "Comparing with X Equal, Y Bigger, and Z Equal with operator >= should return false";
	EXPECT_TRUE(value >= xEqualYLowerZEqual) << "Comparing with X Equal, Y Lower, and Z Equal with operator >= should return true";
	EXPECT_FALSE(value >= xBiggerYEqualZEqual) << "Comparing with X Bigger, Y Equal, and Z Equal with operator >= should return false";
	EXPECT_TRUE(value >= xLowerYEqualZEqual) << "Comparing with X Lower, Y Equal, and Z Equal with operator >= should return true";
}

TEST_F(Vector3Test, ToStringMethod)
{
	EXPECT_EQ(spk::IVector3<int>(3, -4, 5).to_string(), "(3, -4, 5)") << "to_string method should return correct string representation for int";
	EXPECT_EQ(spk::IVector3<float>(3.5f, 4.5f, 5.5f).to_string(), "(3.5, 4.5, 5.5)") << "to_string method should return correct string representation for float";
	EXPECT_EQ(spk::IVector3<double>(3.5, 4.5, 5.5).to_string(), "(3.5, 4.5, 5.5)") << "to_string method should return correct string representation for double";
	EXPECT_EQ(spk::IVector3<size_t>(3, 4, 5).to_string(), "(3, 4, 5)") << "to_string method should return correct string representation for size_t";

	EXPECT_EQ(spk::IVector3<int>(3, -4, 5).to_wstring(), L"(3, -4, 5)") << "to_string method should return correct string representation for int";
	EXPECT_EQ(spk::IVector3<float>(3.5f, 4.5f, 5.5f).to_wstring(), L"(3.5, 4.5, 5.5)") << "to_string method should return correct string representation for float";
	EXPECT_EQ(spk::IVector3<double>(3.5, 4.5, 5.5).to_wstring(), L"(3.5, 4.5, 5.5)") << "to_string method should return correct string representation for double";
	EXPECT_EQ(spk::IVector3<size_t>(3, 4, 5).to_wstring(), L"(3, 4, 5)") << "to_string method should return correct string representation for size_t";
}

TEST_F(Vector3Test, ArithmeticOperators)
{
	spk::IVector3<int> vecInt1(3, 4, 5);
	spk::IVector3<int> vecInt2(1, 2, 3);

	auto vecIntResult = vecInt1 + vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(4, 6, 8)) << "Addition operator should add correctly for int";

	vecIntResult = vecInt1 - vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(2, 2, 2)) << "Subtraction operator should subtract correctly for int";

	vecIntResult = vecInt1 * vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(3, 8, 15)) << "Multiplication operator should multiply correctly for int";

	vecIntResult = vecInt1 / vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(3, 2, 1)) << "Division operator should divide correctly for int";

	spk::IVector3<float> vecFloat1(3.0f, 4.0f, 5.0f);
	spk::IVector3<float> vecFloat2(1.0f, 2.0f, 3.0f);

	auto vecFloatResult = vecFloat1 + vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(4.0f, 6.0f, 8.0f)) << "Addition operator should add correctly for float";

	vecFloatResult = vecFloat1 - vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(2.0f, 2.0f, 2.0f)) << "Subtraction operator should subtract correctly for float";

	vecFloatResult = vecFloat1 * vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(3.0f, 8.0f, 15.0f)) << "Multiplication operator should multiply correctly for float";

	vecFloatResult = vecFloat1 / vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(3.0f, 2.0f, 5.0f / 3.0f)) << "Division operator should divide correctly for float";
}

TEST_F(Vector3Test, CompoundAssignmentOperators)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	spk::IVector3<int> vecIntOther(1, 2, 3);

	vecInt += vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector3<int>(4, 6, 8)) << "Compound addition operator should add correctly for int";

	vecInt -= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector3<int>(3, 4, 5)) << "Compound subtraction operator should subtract correctly for int";

	vecInt *= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector3<int>(3, 8, 15)) << "Compound multiplication operator should multiply correctly for int";

	vecInt /= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector3<int>(3, 4, 5)) << "Compound division operator should divide correctly for int";

	spk::IVector3<float> vecFloat(3.0f, 4.0f, 5.0f);
	spk::IVector3<float> vecFloatOther(1.0f, 2.0f, 3.0f);

	vecFloat += vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(4.0f, 6.0f, 8.0f)) << "Compound addition operator should add correctly for float";

	vecFloat -= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(3.0f, 4.0f, 5.0f)) << "Compound subtraction operator should subtract correctly for float";

	vecFloat *= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(3.0f, 8.0f, 15.0f)) << "Compound multiplication operator should multiply correctly for float";

	vecFloat /= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(3.0f, 4.0f, 5.0f)) << "Compound division operator should divide correctly for float";
}

TEST_F(Vector3Test, ArithmeticOperatorsWithScalar)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	int scalar = 2;

	auto vecIntResult = vecInt + scalar;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(5, 6, 7)) << "Addition operator with scalar should add correctly for int";

	vecIntResult = vecInt - scalar;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(1, 2, 3)) << "Subtraction operator with scalar should subtract correctly for int";

	vecIntResult = vecInt * scalar;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(6, 8, 10)) << "Multiplication operator with scalar should multiply correctly for int";

	vecIntResult = vecInt / scalar;
	EXPECT_EQ(vecIntResult, spk::IVector3<int>(1, 2, 2)) << "Division operator with scalar should divide correctly for int";

	spk::IVector3<float> vecFloat(3.0f, 4.0f, 5.0f);
	float scalarFloat = 2.0f;

	auto vecFloatResult = vecFloat + scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(5.0f, 6.0f, 7.0f)) << "Addition operator with scalar should add correctly for float";

	vecFloatResult = vecFloat - scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(1.0f, 2.0f, 3.0f)) << "Subtraction operator with scalar should subtract correctly for float";

	vecFloatResult = vecFloat * scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(6.0f, 8.0f, 10.0f)) << "Multiplication operator with scalar should multiply correctly for float";

	vecFloatResult = vecFloat / scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector3<float>(1.5f, 2.0f, 2.5f)) << "Division operator with scalar should divide correctly for float";
}

TEST_F(Vector3Test, CompoundAssignmentOperatorsWithScalar)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	int scalar = 2;

	vecInt += scalar;
	EXPECT_EQ(vecInt, spk::IVector3<int>(5, 6, 7)) << "Compound addition operator with scalar should add correctly for int";

	vecInt -= scalar;
	EXPECT_EQ(vecInt, spk::IVector3<int>(3, 4, 5)) << "Compound subtraction operator with scalar should subtract correctly for int";

	vecInt *= scalar;
	EXPECT_EQ(vecInt, spk::IVector3<int>(6, 8, 10)) << "Compound multiplication operator with scalar should multiply correctly for int";

	vecInt /= scalar;
	EXPECT_EQ(vecInt, spk::IVector3<int>(3, 4, 5)) << "Compound division operator with scalar should divide correctly for int";

	spk::IVector3<float> vecFloat(3.0f, 4.0f, 5.0f);
	float scalarFloat = 2.0f;

	vecFloat += scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(5.0f, 6.0f, 7.0f)) << "Compound addition operator with scalar should add correctly for float";

	vecFloat -= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(3.0f, 4.0f, 5.0f)) << "Compound subtraction operator with scalar should subtract correctly for float";

	vecFloat *= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(6.0f, 8.0f, 10.0f)) << "Compound multiplication operator with scalar should multiply correctly for float";

	vecFloat /= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector3<float>(3.0f, 4.0f, 5.0f)) << "Compound division operator with scalar should divide correctly for float";
}

TEST_F(Vector3Test, DivisionByZero)
{
	spk::IVector3<int> vecInt(3, 4, 5);
	int zeroInt = 0;

	EXPECT_THROW(
		{
			spk::IVector3<int> result = vecInt / zeroInt;
		}, std::runtime_error) << "Division by zero for int should throw runtime_error";

	EXPECT_THROW(
		{
			vecInt /= zeroInt;
		}, std::runtime_error) << "Division assignment by zero for int should throw runtime_error";

	spk::IVector3<float> vecFloat(3.0f, 4.0f, 5.0f);
	float zeroFloat = 0.0f;

	EXPECT_THROW(
		{
			spk::IVector3<float> result = vecFloat / zeroFloat;
		}, std::runtime_error) << "Division by zero for float should throw runtime_error";

	EXPECT_THROW(
		{
			vecFloat /= zeroFloat;
		}, std::runtime_error) << "Division assignment by zero for float should throw runtime_error";

	spk::IVector3<double> vecDouble(3.0, 4.0, 5.0);
	double zeroDouble = 0.0;

	EXPECT_THROW(
		{
			spk::IVector3<double> result = vecDouble / zeroDouble;
		}, std::runtime_error) << "Division by zero for double should throw runtime_error";

	EXPECT_THROW(
		{
			vecDouble /= zeroDouble;
		}, std::runtime_error) << "Division assignment by zero for double should throw runtime_error";
}

TEST_F(Vector3Test, DistanceMethod)
{
	spk::IVector3<int> vec1(1, 2, 3);
	spk::IVector3<int> vec2(4, 5, 6);
	EXPECT_FLOAT_EQ(vec1.distance(vec2), static_cast<float>(std::sqrt(27))) << "Distance method should return the correct distance between two vectors";

	spk::IVector3<float> vecFloat1(1.0f, 2.0f, 3.0f);
	spk::IVector3<float> vecFloat2(4.0f, 5.0f, 6.0f);
	EXPECT_FLOAT_EQ(vecFloat1.distance(vecFloat2), static_cast<float>(std::sqrt(27.0f))) << "Distance method should return the correct distance between two float vectors";

	spk::IVector3<double> vecDouble1(1.0, 2.0, 3.0);
	spk::IVector3<double> vecDouble2(4.0, 5.0, 6.0);
	EXPECT_FLOAT_EQ(vecDouble1.distance(vecDouble2), static_cast<float>(std::sqrt(27.0))) << "Distance method should return the correct distance between two double vectors";
}

TEST_F(Vector3Test, NormMethod)
{
	spk::IVector3<int> vec(1, 2, 2);
	EXPECT_FLOAT_EQ(vec.norm(), 3.0f) << "Norm method should return the correct norm of the vector";

	spk::IVector3<float> vecFloat(1.0f, 2.0f, 2.0f);
	EXPECT_FLOAT_EQ(vecFloat.norm(), 3.0f) << "Norm method should return the correct norm of the float vector";

	spk::IVector3<double> vecDouble(1.0, 2.0, 2.0);
	EXPECT_DOUBLE_EQ(vecDouble.norm(), 3.0) << "Norm method should return the correct norm of the double vector";
}

TEST_F(Vector3Test, NormalizeMethod)
{
	spk::IVector3<int> vec(3, 0, 4);
	auto normVec = vec.normalize();
	EXPECT_EQ(normVec, spk::IVector3<float>(0.6f, 0.0f, 0.8f)) << "Normalize method should return the correct normalized vector";

	spk::IVector3<float> vecFloat(3.0f, 0.0f, 4.0f);
	auto normVecFloat = vecFloat.normalize();
	EXPECT_EQ(normVecFloat, spk::IVector3<float>(0.6f, 0.0f, 0.8f)) << "Normalize method should return the correct normalized vector for float";

	spk::IVector3<double> vecDouble(3.0, 0.0, 4.0);
	auto normVecDouble = vecDouble.normalize();
	EXPECT_EQ(normVecDouble, spk::IVector3<double>(0.6, 0.0, 0.8)) << "Normalize method should return the correct normalized vector for double";
}

TEST_F(Vector3Test, CrossMethod)
{
	spk::IVector3<int> vec1(1, 2, 3);
	spk::IVector3<int> vec2(4, 5, 6);
	auto crossVec = vec1.cross(vec2);
	EXPECT_EQ(crossVec, spk::IVector3<int>(-3, 6, -3)) << "Cross product method should return the correct vector";

	spk::IVector3<float> vecFloat1(1.0f, 2.0f, 3.0f);
	spk::IVector3<float> vecFloat2(4.0f, 5.0f, 6.0f);
	auto crossVecFloat = vecFloat1.cross(vecFloat2);
	EXPECT_EQ(crossVecFloat, spk::IVector3<float>(-3.0f, 6.0f, -3.0f)) << "Cross product method should return the correct vector for float";

	spk::IVector3<double> vecDouble1(1.0, 2.0, 3.0);
	spk::IVector3<double> vecDouble2(4.0, 5.0, 6.0);
	auto crossVecDouble = vecDouble1.cross(vecDouble2);
	EXPECT_EQ(crossVecDouble, spk::IVector3<double>(-3.0, 6.0, -3.0)) << "Cross product method should return the correct vector for double";
}

TEST_F(Vector3Test, RotateMethod)
{
	spk::IVector3<float> vec(1.0f, 0.0f, 0.0f);
	spk::IVector3<float> rotationValues(0.0f, 0.0f, 90.0f);
	auto rotatedVec = vec.rotate(rotationValues);
	EXPECT_NEAR(rotatedVec.x, 0.0f, 1e-6) << "Rotate method should return the correct rotated x value";
	EXPECT_NEAR(rotatedVec.y, 1.0f, 1e-6) << "Rotate method should return the correct rotated y value";
	EXPECT_NEAR(rotatedVec.z, 0.0f, 1e-6) << "Rotate method should return the correct rotated z value";

	spk::IVector3<double> vecDouble(1.0, 0.0, 0.0);
	spk::IVector3<double> rotationValuesDouble(0.0, 0.0, 90.0);
	auto rotatedVecDouble = vecDouble.rotate(rotationValuesDouble);
	EXPECT_NEAR(rotatedVecDouble.x, 0.0, 1e-6) << "Rotate method should return the correct rotated x value for double";
	EXPECT_NEAR(rotatedVecDouble.y, 1.0, 1e-6) << "Rotate method should return the correct rotated y value for double";
	EXPECT_NEAR(rotatedVecDouble.z, 0.0, 1e-6) << "Rotate method should return the correct rotated z value for double";
}

TEST_F(Vector3Test, DotMethod)
{
	spk::IVector3<int> vec1(1, 2, 3);
	spk::IVector3<int> vec2(4, 5, 6);
	EXPECT_EQ(vec1.dot(vec2), 32) << "Dot product method should return the correct dot product";

	spk::IVector3<float> vecFloat1(1.0f, 2.0f, 3.0f);
	spk::IVector3<float> vecFloat2(4.0f, 5.0f, 6.0f);
	EXPECT_FLOAT_EQ(vecFloat1.dot(vecFloat2), 32.0f) << "Dot product method should return the correct dot product for float";

	spk::IVector3<double> vecDouble1(1.0, 2.0, 3.0);
	spk::IVector3<double> vecDouble2(4.0, 5.0, 6.0);
	EXPECT_DOUBLE_EQ(vecDouble1.dot(vecDouble2), 32.0) << "Dot product method should return the correct dot product for double";
}

TEST_F(Vector3Test, InverseMethod)
{
	spk::IVector3<int> vec(1, -2, 3);
	auto invVec = vec.inverse();
	EXPECT_EQ(invVec, spk::IVector3<int>(-1, 2, -3)) << "Inverse method should return the correct inverted vector";
}

TEST_F(Vector3Test, XYMethod)
{
	spk::IVector3<int> vec(1, 2, 3);
	auto xyVec = vec.xy();
	EXPECT_EQ(xyVec, spk::IVector2<int>(1, 2)) << "XY method should return the correct 2D vector with X and Y components";
}

TEST_F(Vector3Test, XZMethod)
{
	spk::IVector3<int> vec(1, 2, 3);
	auto xzVec = vec.xz();
	EXPECT_EQ(xzVec, spk::IVector2<int>(1, 3)) << "XZ method should return the correct 2D vector with X and Z components";
}

TEST_F(Vector3Test, YZMethod)
{
	spk::IVector3<int> vec(1, 2, 3);
	auto yzVec = vec.yz();
	EXPECT_EQ(yzVec, spk::IVector2<int>(2, 3)) << "YZ method should return the correct 2D vector with Y and Z components";
}

TEST_F(Vector3Test, RadianToDegreeMethod)
{
	spk::IVector3<float> radians(3.14159f, 3.14159f / 2, 3.14159f / 4);
	auto degrees = spk::IVector3<float>::radianToDegree(radians);
	EXPECT_NEAR(degrees.x, 180.0f, 0.001f) << "Radian to degree conversion should return the correct degree value";
	EXPECT_NEAR(degrees.y, 90.0f, 0.001f) << "Radian to degree conversion should return the correct degree value";
	EXPECT_NEAR(degrees.z, 45.0f, 0.001f) << "Radian to degree conversion should return the correct degree value";
}

TEST_F(Vector3Test, DegreeToRadianMethod)
{
	spk::IVector3<float> degrees(180.0f, 90.0f, 45.0f);
	auto radians = spk::IVector3<float>::degreeToRadian(degrees);
	EXPECT_NEAR(radians.x, 3.14159f, 0.001f) << "Degree to radian conversion should return the correct radian value";
	EXPECT_NEAR(radians.y, 3.14159f / 2, 0.001f) << "Degree to radian conversion should return the correct radian value";
	EXPECT_NEAR(radians.z, 3.14159f / 4, 0.001f) << "Degree to radian conversion should return the correct radian value";
}

TEST_F(Vector3Test, ClampMethod)
{
	spk::IVector3<int> vec(5, 10, 15);
	spk::IVector3<int> lower(0, 5, 10);
	spk::IVector3<int> upper(10, 15, 20);
	auto clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(5, 10, 15)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(-5, 10, 15);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(0, 10, 15)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(15, 10, 15);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(10, 10, 15)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(5, 0, 15);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(5, 5, 15)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(5, 20, 15);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(5, 15, 15)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(5, 10, 5);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(5, 10, 10)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector3<int>(5, 10, 25);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector3<int>(5, 10, 20)) << "Clamp method should return the correct clamped vector";
}

TEST_F(Vector3Test, FloorMethod)
{
	spk::IVector3<float> vec(1.5f, 2.7f, 3.9f);
	auto flooredVec = spk::IVector3<float>::floor(vec);
	EXPECT_EQ(flooredVec, spk::IVector3<int>(1, 2, 3)) << "Floor method should return the correct floored vector";
}

TEST_F(Vector3Test, CeilingMethod)
{
	spk::IVector3<float> vec(1.1f, 2.3f, 3.6f);
	auto ceilingVec = spk::IVector3<float>::ceiling(vec);
	EXPECT_EQ(ceilingVec, spk::IVector3<int>(2, 3, 4)) << "Ceiling method should return the correct ceiling vector";
}

TEST_F(Vector3Test, RoundMethod)
{
	spk::IVector3<float> vec(1.4f, 2.5f, 3.6f);
	auto roundedVec = spk::IVector3<float>::round(vec);
	EXPECT_EQ(roundedVec, spk::IVector3<int>(1, 3, 4)) << "Round method should return the correct rounded vector";
}

TEST_F(Vector3Test, MinMethod)
{
	spk::IVector3<int> vec1(1, 4, 3);
	spk::IVector3<int> vec2(2, 3, 4);
	auto minVec = spk::IVector3<int>::min(vec1, vec2);
	EXPECT_EQ(minVec, spk::IVector3<int>(1, 3, 3)) << "Min method should return the correct component-wise minimum vector";
}

TEST_F(Vector3Test, MaxMethod)
{
	spk::IVector3<int> vec1(1, 4, 3);
	spk::IVector3<int> vec2(2, 3, 4);
	auto maxVec = spk::IVector3<int>::max(vec1, vec2);
	EXPECT_EQ(maxVec, spk::IVector3<int>(2, 4, 4)) << "Max method should return the correct component-wise maximum vector";
}

TEST_F(Vector3Test, LerpMethod)
{
	spk::IVector3<int> start(0, 0, 0);
	spk::IVector3<int> end(10, 10, 10);
	auto lerpVec = spk::IVector3<int>::lerp(start, end, 0.5f);
	EXPECT_EQ(lerpVec, spk::IVector3<int>(5, 5, 5)) << "Lerp method should return the correct linearly interpolated vector";
}
