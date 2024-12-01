#include "structure/math/spk_vector2_tester.hpp"

TEST_F(Vector2Test, DefaultConstructor)
{
	spk::IVector2<int> vecInt;
	EXPECT_EQ(vecInt.x, 0) << "Default constructor for int should initialize x to 0";
	EXPECT_EQ(vecInt.y, 0) << "Default constructor for int should initialize y to 0";

	spk::IVector2<float> vecFloat;
	EXPECT_EQ(vecFloat.x, 0.0f) << "Default constructor for float should initialize x to 0.0f";
	EXPECT_EQ(vecFloat.y, 0.0f) << "Default constructor for float should initialize y to 0.0f";

	spk::IVector2<double> vecDouble;
	EXPECT_EQ(vecDouble.x, 0.0) << "Default constructor for double should initialize x to 0.0";
	EXPECT_EQ(vecDouble.y, 0.0) << "Default constructor for double should initialize y to 0.0";

	spk::IVector2<size_t> vecSizeT;
	EXPECT_EQ(vecSizeT.x, 0) << "Default constructor for size_t should initialize x to 0";
	EXPECT_EQ(vecSizeT.y, 0) << "Default constructor for size_t should initialize y to 0";
}

TEST_F(Vector2Test, ScalarConstructor)
{
	spk::IVector2<int> vecInt(5);
	EXPECT_EQ(vecInt.x, 5) << "Scalar constructor for int should initialize x to 5";
	EXPECT_EQ(vecInt.y, 5) << "Scalar constructor for int should initialize y to 5";

	spk::IVector2<float> vecFloat(5.5f);
	EXPECT_FLOAT_EQ(vecFloat.x, 5.5f) << "Scalar constructor for float should initialize x to 5.5f";
	EXPECT_FLOAT_EQ(vecFloat.y, 5.5f) << "Scalar constructor for float should initialize y to 5.5f";

	spk::IVector2<double> vecDouble(5.5);
	EXPECT_DOUBLE_EQ(vecDouble.x, 5.5) << "Scalar constructor for double should initialize x to 5.5";
	EXPECT_DOUBLE_EQ(vecDouble.y, 5.5) << "Scalar constructor for double should initialize y to 5.5";

	spk::IVector2<size_t> vecSizeT(5);
	EXPECT_EQ(vecSizeT.x, 5) << "Scalar constructor for size_t should initialize x to 5";
	EXPECT_EQ(vecSizeT.y, 5) << "Scalar constructor for size_t should initialize y to 5";
}

TEST_F(Vector2Test, TwoValueConstructor)
{
	spk::IVector2<int> vecInt(3, 4);
	EXPECT_EQ(vecInt.x, 3) << "Two-value constructor for int should initialize x to 3";
	EXPECT_EQ(vecInt.y, 4) << "Two-value constructor for int should initialize y to 4";

	spk::IVector2<float> vecFloat(3.5f, 4.5f);
	EXPECT_FLOAT_EQ(vecFloat.x, 3.5f) << "Two-value constructor for float should initialize x to 3.5f";
	EXPECT_FLOAT_EQ(vecFloat.y, 4.5f) << "Two-value constructor for float should initialize y to 4.5f";

	spk::IVector2<double> vecDouble(3.5, 4.5);
	EXPECT_DOUBLE_EQ(vecDouble.x, 3.5) << "Two-value constructor for double should initialize x to 3.5";
	EXPECT_DOUBLE_EQ(vecDouble.y, 4.5) << "Two-value constructor for double should initialize y to 4.5";

	spk::IVector2<size_t> vecSizeT(3, 4);
	EXPECT_EQ(vecSizeT.x, 3) << "Two-value constructor for size_t should initialize x to 3";
	EXPECT_EQ(vecSizeT.y, 4) << "Two-value constructor for size_t should initialize y to 4";
}

TEST_F(Vector2Test, CopyConstructor)
{
	spk::IVector2<int> vecInt1(3, 4);
	spk::IVector2<int> vecInt2(vecInt1);
	EXPECT_EQ(vecInt2.x, 3) << "Copy constructor for int should copy x value";
	EXPECT_EQ(vecInt2.y, 4) << "Copy constructor for int should copy y value";

	spk::IVector2<float> vecFloat1(3.5f, 4.5f);
	spk::IVector2<float> vecFloat2(vecFloat1);
	EXPECT_FLOAT_EQ(vecFloat2.x, 3.5f) << "Copy constructor for float should copy x value";
	EXPECT_FLOAT_EQ(vecFloat2.y, 4.5f) << "Copy constructor for float should copy y value";

	spk::IVector2<double> vecDouble1(3.5, 4.5);
	spk::IVector2<double> vecDouble2(vecDouble1);
	EXPECT_DOUBLE_EQ(vecDouble2.x, 3.5) << "Copy constructor for double should copy x value";
	EXPECT_DOUBLE_EQ(vecDouble2.y, 4.5) << "Copy constructor for double should copy y value";

	spk::IVector2<size_t> vecSizeT1(3, 4);
	spk::IVector2<size_t> vecSizeT2(vecSizeT1);
	EXPECT_EQ(vecSizeT2.x, 3) << "Copy constructor for size_t should copy x value";
	EXPECT_EQ(vecSizeT2.y, 4) << "Copy constructor for size_t should copy y value";
}

TEST_F(Vector2Test, EqualityOperator)
{
	spk::IVector2<int> vecInt1(3, 4);
	spk::IVector2<int> vecInt2(3, 4);
	spk::IVector2<int> vecInt3(5, 6);
	EXPECT_EQ(vecInt1, vecInt2) << "Equality operator should return true for equal int vectors";
	EXPECT_NE(vecInt1, vecInt3) << "Equality operator should return false for different int vectors";

	spk::IVector2<float> vecFloat1(3.0f, 4.0f);
	spk::IVector2<float> vecFloat2(3.0f, 4.0f);
	spk::IVector2<float> vecFloat3(5.0f, 6.0f);
	EXPECT_EQ(vecFloat1, vecFloat2) << "Equality operator should return true for equal float vectors";
	EXPECT_NE(vecFloat1, vecFloat3) << "Equality operator should return false for different float vectors";

	spk::IVector2<double> vecDouble1(3.0, 4.0);
	spk::IVector2<double> vecDouble2(3.0, 4.0);
	spk::IVector2<double> vecDouble3(5.0, 6.0);
	EXPECT_EQ(vecDouble1, vecDouble2) << "Equality operator should return true for equal double vectors";
	EXPECT_NE(vecDouble1, vecDouble3) << "Equality operator should return false for different double vectors";

	spk::IVector2<size_t> vecSizeT1(3, 4);
	spk::IVector2<size_t> vecSizeT2(3, 4);
	spk::IVector2<size_t> vecSizeT3(5, 6);
	EXPECT_EQ(vecSizeT1, vecSizeT2) << "Equality operator should return true for equal size_t vectors";
	EXPECT_NE(vecSizeT1, vecSizeT3) << "Equality operator should return false for different size_t vectors";

	spk::IVector2<int> vecInt4(3, 4);
	spk::IVector2<float> vecFloat4(3.5f, 4.0f);

	EXPECT_TRUE(vecInt4 == vecFloat4) << "Upon casting float to int, vecFloat should be equal as 3.5 should be concidered as 3";
	EXPECT_FALSE(vecFloat4 == vecInt4) << "Upon casting int to float, vecInt should be equal as 3 should be concidered as 3.0 and therefor must be different";
}

TEST_F(Vector2Test, MixedTypeEqualityOperator)
{
	spk::IVector2<int> vecInt(3, 4);
	spk::IVector2<float> vecFloat(3.0f, 4.0f);
	spk::IVector2<double> vecDouble(3.0, 4.0);
	spk::IVector2<size_t> vecSizeT(3, 4);

	EXPECT_EQ(vecInt, vecFloat) << "Mixed-type equality operator should return true for equivalent int and float vectors";
	EXPECT_EQ(vecInt, vecDouble) << "Mixed-type equality operator should return true for equivalent int and double vectors";
	EXPECT_EQ(vecInt, vecSizeT) << "Mixed-type equality operator should return true for equivalent int and size_t vectors";
	EXPECT_EQ(vecFloat, vecDouble) << "Mixed-type equality operator should return true for equivalent float and double vectors";
	EXPECT_EQ(vecFloat, vecSizeT) << "Mixed-type equality operator should return true for equivalent float and size_t vectors";
	EXPECT_EQ(vecDouble, vecSizeT) << "Mixed-type equality operator should return true for equivalent double and size_t vectors";

	spk::IVector2<int> vecIntDifferent(5, 6);
	spk::IVector2<float> vecFloatDifferent(5.0f, 6.0f);
	spk::IVector2<double> vecDoubleDifferent(5.0, 6.0);
	spk::IVector2<size_t> vecSizeTDifferent(5, 6);

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

TEST_F(Vector2Test, ComparatorOperators)
{
	spk::IVector2<int> value(5, 6);
	spk::IVector2<int> xEqualYEqual(5, 6);
	spk::IVector2<int> xEqualYBigger(5, 7);
	spk::IVector2<int> xEqualYLower(5, 5);
	spk::IVector2<int> xBiggerYEqual(6, 6);
	spk::IVector2<int> xLowerYEqual(4, 6);

	EXPECT_FALSE(value < xEqualYEqual) << "Comparing with X Equal and Y Bigger with operator < should return false";
	EXPECT_TRUE(value < xEqualYBigger) << "Comparing with X Equal and Y Bigger with operator < should return false";
	EXPECT_FALSE(value < xEqualYLower) << "Comparing with X Equal and Y Bigger with operator < should return true";
	EXPECT_TRUE(value < xBiggerYEqual) << "Comparing with X Equal and Y Bigger with operator < should return false";
	EXPECT_FALSE(value < xLowerYEqual) << "Comparing with X Equal and Y Bigger with operator < should return true";

	EXPECT_FALSE(value > xEqualYEqual) << "Comparing with X Equal and Y Bigger with operator > should return false";
	EXPECT_FALSE(value > xEqualYBigger) << "Comparing with X Equal and Y Bigger with operator > should return false";
	EXPECT_TRUE(value > xEqualYLower) << "Comparing with X Equal and Y Bigger with operator > should return true";
	EXPECT_FALSE(value > xBiggerYEqual) << "Comparing with X Equal and Y Bigger with operator > should return false";
	EXPECT_TRUE(value > xLowerYEqual) << "Comparing with X Equal and Y Bigger with operator > should return true";

	EXPECT_TRUE(value <= xEqualYEqual) << "Comparing with X Equal and Y Bigger with operator <= should return false";
	EXPECT_TRUE(value <= xEqualYBigger) << "Comparing with X Equal and Y Bigger with operator <= should return false";
	EXPECT_FALSE(value <= xEqualYLower) << "Comparing with X Equal and Y Bigger with operator <= should return true";
	EXPECT_TRUE(value <= xBiggerYEqual) << "Comparing with X Equal and Y Bigger with operator <= should return false";
	EXPECT_FALSE(value <= xLowerYEqual) << "Comparing with X Equal and Y Bigger with operator <= should return true";
	
	EXPECT_TRUE(value >= xEqualYEqual) << "Comparing with X Equal and Y Bigger with operator >= should return false";
	EXPECT_FALSE(value >= xEqualYBigger) << "Comparing with X Equal and Y Bigger with operator >= should return false";
	EXPECT_TRUE(value >= xEqualYLower) << "Comparing with X Equal and Y Bigger with operator >= should return true";
	EXPECT_FALSE(value >= xBiggerYEqual) << "Comparing with X Equal and Y Bigger with operator >= should return false";
	EXPECT_TRUE(value >= xLowerYEqual) << "Comparing with X Equal and Y Bigger with operator >= should return true";
}

TEST_F(Vector2Test, ToStringMethod)
{
	EXPECT_EQ(spk::IVector2<int>(3, -4).to_string(), "(3, -4)") << "to_string method should return correct string representation for int";
	EXPECT_EQ(spk::IVector2<float>(3.5f, 4.5f).to_string(), "(3.5, 4.5)") << "to_string method should return correct string representation for float";
	EXPECT_EQ(spk::IVector2<double>(3.5, 4.5).to_string(), "(3.5, 4.5)") << "to_string method should return correct string representation for double";
	EXPECT_EQ(spk::IVector2<size_t>(3, 4).to_string(), "(3, 4)") << "to_string method should return correct string representation for size_t";

	EXPECT_EQ(spk::IVector2<int>(3, -4).to_wstring(), L"(3, -4)") << "to_string method should return correct string representation for int";
	EXPECT_EQ(spk::IVector2<float>(3.5f, 4.5f).to_wstring(), L"(3.5, 4.5)") << "to_string method should return correct string representation for float";
	EXPECT_EQ(spk::IVector2<double>(3.5, 4.5).to_wstring(), L"(3.5, 4.5)") << "to_string method should return correct string representation for double";
	EXPECT_EQ(spk::IVector2<size_t>(3, 4).to_wstring(), L"(3, 4)") << "to_string method should return correct string representation for size_t";
}

TEST_F(Vector2Test, ArithmeticOperators)
{
	spk::IVector2<int> vecInt1(3, 4);
	spk::IVector2<int> vecInt2(1, 2);

	auto vecIntResult = vecInt1 + vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(4, 6)) << "Addition operator should add correctly for int";

	vecIntResult = vecInt1 - vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(2, 2)) << "Subtraction operator should subtract correctly for int";

	vecIntResult = vecInt1 * vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(3, 8)) << "Multiplication operator should multiply correctly for int";

	vecIntResult = vecInt1 / vecInt2;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(3, 2)) << "Division operator should divide correctly for int";

	spk::IVector2<float> vecFloat1(3.0f, 4.0f);
	spk::IVector2<float> vecFloat2(1.0f, 2.0f);

	auto vecFloatResult = vecFloat1 + vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(4.0f, 6.0f)) << "Addition operator should add correctly for float";

	vecFloatResult = vecFloat1 - vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(2.0f, 2.0f)) << "Subtraction operator should subtract correctly for float";

	vecFloatResult = vecFloat1 * vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(3.0f, 8.0f)) << "Multiplication operator should multiply correctly for float";

	vecFloatResult = vecFloat1 / vecFloat2;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(3.0f, 2.0f)) << "Division operator should divide correctly for float";
}

TEST_F(Vector2Test, CompoundAssignmentOperators)
{
	spk::IVector2<int> vecInt(3, 4);
	spk::IVector2<int> vecIntOther(1, 2);

	vecInt += vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector2<int>(4, 6)) << "Compound addition operator should add correctly for int";

	vecInt -= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector2<int>(3, 4)) << "Compound subtraction operator should subtract correctly for int";

	vecInt *= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector2<int>(3, 8)) << "Compound multiplication operator should multiply correctly for int";

	vecInt /= vecIntOther;
	EXPECT_EQ(vecInt, spk::IVector2<int>(3, 4)) << "Compound division operator should divide correctly for int";

	spk::IVector2<float> vecFloat(3.0f, 4.0f);
	spk::IVector2<float> vecFloatOther(1.0f, 2.0f);

	vecFloat += vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(4.0f, 6.0f)) << "Compound addition operator should add correctly for float";

	vecFloat -= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(3.0f, 4.0f)) << "Compound subtraction operator should subtract correctly for float";

	vecFloat *= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(3.0f, 8.0f)) << "Compound multiplication operator should multiply correctly for float";

	vecFloat /= vecFloatOther;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(3.0f, 4.0f)) << "Compound division operator should divide correctly for float";
}

TEST_F(Vector2Test, ArithmeticOperatorsWithScalar)
{
	spk::IVector2<int> vecInt(3, 4);
	int scalar = 2;

	auto vecIntResult = vecInt + scalar;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(5, 6)) << "Addition operator with scalar should add correctly for int";

	vecIntResult = vecInt - scalar;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(1, 2)) << "Subtraction operator with scalar should subtract correctly for int";

	vecIntResult = vecInt * scalar;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(6, 8)) << "Multiplication operator with scalar should multiply correctly for int";

	vecIntResult = vecInt / scalar;
	EXPECT_EQ(vecIntResult, spk::IVector2<int>(1, 2)) << "Division operator with scalar should divide correctly for int";

	spk::IVector2<float> vecFloat(3.0f, 4.0f);
	float scalarFloat = 2.0f;

	auto vecFloatResult = vecFloat + scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(5.0f, 6.0f)) << "Addition operator with scalar should add correctly for float";

	vecFloatResult = vecFloat - scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(1.0f, 2.0f)) << "Subtraction operator with scalar should subtract correctly for float";

	vecFloatResult = vecFloat * scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(6.0f, 8.0f)) << "Multiplication operator with scalar should multiply correctly for float";

	vecFloatResult = vecFloat / scalarFloat;
	EXPECT_EQ(vecFloatResult, spk::IVector2<float>(1.5f, 2.0f)) << "Division operator with scalar should divide correctly for float";
}

TEST_F(Vector2Test, CompoundAssignmentOperatorsWithScalar)
{
	spk::IVector2<int> vecInt(3, 4);
	int scalar = 2;

	vecInt += scalar;
	EXPECT_EQ(vecInt, spk::IVector2<int>(5, 6)) << "Compound addition operator with scalar should add correctly for int";

	vecInt -= scalar;
	EXPECT_EQ(vecInt, spk::IVector2<int>(3, 4)) << "Compound subtraction operator with scalar should subtract correctly for int";

	vecInt *= scalar;
	EXPECT_EQ(vecInt, spk::IVector2<int>(6, 8)) << "Compound multiplication operator with scalar should multiply correctly for int";

	vecInt /= scalar;
	EXPECT_EQ(vecInt, spk::IVector2<int>(3, 4)) << "Compound division operator with scalar should divide correctly for int";

	spk::IVector2<float> vecFloat(3.0f, 4.0f);
	float scalarFloat = 2.0f;

	vecFloat += scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(5.0f, 6.0f)) << "Compound addition operator with scalar should add correctly for float";

	vecFloat -= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(3.0f, 4.0f)) << "Compound subtraction operator with scalar should subtract correctly for float";

	vecFloat *= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(6.0f, 8.0f)) << "Compound multiplication operator with scalar should multiply correctly for float";

	vecFloat /= scalarFloat;
	EXPECT_EQ(vecFloat, spk::IVector2<float>(3.0f, 4.0f)) << "Compound division operator with scalar should divide correctly for float";
}

TEST_F(Vector2Test, DivisionByZero)
{
	spk::IVector2<int> vecInt(3, 4);
	int zeroInt = 0;

	EXPECT_THROW(
		{
			spk::IVector2<int> result = vecInt / zeroInt;
		}, std::runtime_error) << "Division by zero for int should throw runtime_error";

	EXPECT_THROW(
		{
			vecInt /= zeroInt;
		}, std::runtime_error) << "Division assignment by zero for int should throw runtime_error";

	spk::IVector2<float> vecFloat(3.0f, 4.0f);
	float zeroFloat = 0.0f;

	EXPECT_THROW(
		{
			spk::IVector2<float> result = vecFloat / zeroFloat;
		}, std::runtime_error) << "Division by zero for float should throw runtime_error";

	EXPECT_THROW(
		{
			vecFloat /= zeroFloat;
		}, std::runtime_error) << "Division assignment by zero for float should throw runtime_error";

	spk::IVector2<double> vecDouble(3.0, 4.0);
	double zeroDouble = 0.0;

	EXPECT_THROW(
		{
			spk::IVector2<double> result = vecDouble / zeroDouble;
		}, std::runtime_error) << "Division by zero for double should throw runtime_error";

	EXPECT_THROW(
		{
			vecDouble /= zeroDouble;
		}, std::runtime_error) << "Division assignment by zero for double should throw runtime_error";
}

TEST_F(Vector2Test, DistanceMethod)
{
	spk::IVector2<int> vec1(1, 2);
	spk::IVector2<int> vec2(4, 5);
	EXPECT_FLOAT_EQ(vec1.distance(vec2), static_cast<float>(std::sqrt(18))) << "Distance method should return the correct distance between two vectors";

	spk::IVector2<float> vecFloat1(1.0f, 2.0f);
	spk::IVector2<float> vecFloat2(4.0f, 5.0f);
	EXPECT_FLOAT_EQ(vecFloat1.distance(vecFloat2), static_cast<float>(std::sqrt(18.0f))) << "Distance method should return the correct distance between two float vectors";

	spk::IVector2<double> vecDouble1(1.0, 2.0);
	spk::IVector2<double> vecDouble2(4.0, 5.0);
	EXPECT_FLOAT_EQ(vecDouble1.distance(vecDouble2), static_cast<float>(std::sqrt(18.0))) << "Distance method should return the correct distance between two double vectors";
}

TEST_F(Vector2Test, NormMethod)
{
	spk::IVector2<int> vec(1, 1);
	EXPECT_FLOAT_EQ(vec.norm(), static_cast<float>(std::sqrt(2))) << "Norm method should return the correct norm of the vector";

	spk::IVector2<float> vecFloat(1.0f, 1.0f);
	EXPECT_FLOAT_EQ(vecFloat.norm(), static_cast<float>(std::sqrt(2))) << "Norm method should return the correct norm of the float vector";

	spk::IVector2<double> vecDouble(1.0, 1.0);
	EXPECT_DOUBLE_EQ(vecDouble.norm(), static_cast<float>(std::sqrt(2))) << "Norm method should return the correct norm of the double vector";
}

TEST_F(Vector2Test, NormalizeMethod)
{
	spk::IVector2<int> vec(15, 10);
	auto normVec = vec.normalize();
	auto expected = spk::IVector2<float>(0.83205f, 0.55470f);
	EXPECT_EQ(normVec, expected) << "Normalize method should return the correct normalized vector";

	spk::IVector2<float> vecFloat(15.0f, 10.0f);
	auto normVecFloat = vecFloat.normalize();
	EXPECT_EQ(normVecFloat, expected) << "Normalize method should return the correct normalized vector for float";

	spk::IVector2<double> vecDouble(15.0, 10.0);
	auto normVecDouble = vecDouble.normalize();
	EXPECT_EQ(normVecDouble, expected) << "Normalize method should return the correct normalized vector for double";
}

TEST_F(Vector2Test, CrossMethod)
{
	spk::IVector2<int> vec1(1, 2);
	auto crossVec = vec1.cross();
	EXPECT_EQ(crossVec, spk::IVector2<int>(-2, 1)) << "Cross product method should return the correct vector";

	spk::IVector2<float> vecFloat1(1.0f, 2.0f);
	auto crossVecFloat = vecFloat1.cross();
	EXPECT_EQ(crossVecFloat, spk::IVector2<float>(-2.0f, 1.0f)) << "Cross product method should return the correct vector for float";

	spk::IVector2<double> vecDouble1(1.0, 2.0);
	auto crossVecDouble = vecDouble1.cross();
	EXPECT_EQ(crossVecDouble, spk::IVector2<double>(-2.0, 1.0)) << "Cross product method should return the correct vector for double";
}

TEST_F(Vector2Test, RotateMethod)
{
	spk::IVector2<float> vec(1.0f, 0.0f);
	float rotationValues(90.0f);
	auto rotatedVec = vec.rotate(rotationValues);
	EXPECT_NEAR(rotatedVec.x, 0.0f, 1e-6) << "Rotate method should return the correct rotated x value";
	EXPECT_NEAR(rotatedVec.y, 1.0f, 1e-6) << "Rotate method should return the correct rotated y value";

	spk::IVector2<double> vecDouble(1.0, 0.0);
	double rotationValuesDouble(90);
	auto rotatedVecDouble = vecDouble.rotate(rotationValuesDouble);
	EXPECT_NEAR(rotatedVecDouble.x, 0.0, 1e-6) << "Rotate method should return the correct rotated x value for double";
	EXPECT_NEAR(rotatedVecDouble.y, 1.0, 1e-6) << "Rotate method should return the correct rotated y value for double";
}

TEST_F(Vector2Test, DotMethod)
{
	spk::IVector2<int> vec1(1, 2);
	spk::IVector2<int> vec2(4, 5);
	EXPECT_EQ(vec1.dot(vec2), 14) << "Dot product method should return the correct dot product";

	spk::IVector2<float> vecFloat1(1.0f, 2.0f);
	spk::IVector2<float> vecFloat2(4.0f, 5.0f);
	EXPECT_FLOAT_EQ(vecFloat1.dot(vecFloat2), 14.0f) << "Dot product method should return the correct dot product for float";

	spk::IVector2<double> vecDouble1(1.0, 2.0);
	spk::IVector2<double> vecDouble2(4.0, 5.0);
	EXPECT_DOUBLE_EQ(vecDouble1.dot(vecDouble2), 14.0) << "Dot product method should return the correct dot product for double";
}

TEST_F(Vector2Test, InverseMethod)
{
	spk::IVector2<int> vec(1, -2);
	auto invVec = vec.inverse();
	EXPECT_EQ(invVec, spk::IVector2<int>(-1, 2)) << "Inverse method should return the correct inverted vector";
}

TEST_F(Vector2Test, RadianToDegreeMethod)
{
	spk::IVector2<float> radians(3.14159f, 3.14159f / 2);
	auto degrees = spk::IVector2<float>::radianToDegree(radians);
	EXPECT_NEAR(degrees.x, 180.0f, 0.001f) << "Radian to degree conversion should return the correct degree value";
	EXPECT_NEAR(degrees.y, 90.0f, 0.001f) << "Radian to degree conversion should return the correct degree value";
}

TEST_F(Vector2Test, DegreeToRadianMethod)
{
	spk::IVector2<float> degrees(180.0f, 90.0f);
	auto radians = spk::IVector2<float>::degreeToRadian(degrees);
	EXPECT_NEAR(radians.x, 3.14159f, 0.001f) << "Degree to radian conversion should return the correct radian value";
	EXPECT_NEAR(radians.y, 3.14159f / 2, 0.001f) << "Degree to radian conversion should return the correct radian value";
}

TEST_F(Vector2Test, ClampMethod)
{
	spk::IVector2<int> vec(5, 10);
	spk::IVector2<int> lower(0, 5);
	spk::IVector2<int> upper(10, 15);
	auto clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector2<int>(5, 10)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector2<int>(-5, 10);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector2<int>(0, 10)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector2<int>(15, 10);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector2<int>(10, 10)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector2<int>(5, 0);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector2<int>(5, 5)) << "Clamp method should return the correct clamped vector";

	vec = spk::IVector2<int>(5, 20);
	clampedVec = vec.clamp(lower, upper);
	EXPECT_EQ(clampedVec, spk::IVector2<int>(5, 15)) << "Clamp method should return the correct clamped vector";
}

TEST_F(Vector2Test, FloorMethod)
{
	spk::IVector2<float> vec(1.5f, 2.7f);
	auto flooredVec = spk::IVector2<float>::floor(vec);
	EXPECT_EQ(flooredVec, spk::IVector2<int>(1, 2)) << "Floor method should return the correct floored vector";
}

TEST_F(Vector2Test, CeilingMethod)
{
	spk::IVector2<float> vec(1.1f, 2.3f);
	auto ceilingVec = spk::IVector2<float>::ceiling(vec);
	EXPECT_EQ(ceilingVec, spk::IVector2<int>(2, 3)) << "Ceiling method should return the correct ceiling vector";
}

TEST_F(Vector2Test, RoundMethod)
{
	spk::IVector2<float> vec(1.4f, 2.6f);
	auto roundedVec = spk::IVector2<float>::round(vec);
	EXPECT_EQ(roundedVec, spk::IVector2<int>(1, 3)) << "Round method should return the correct rounded vector";
}

TEST_F(Vector2Test, MinMethod)
{
	spk::IVector2<int> vec1(1, 4);
	spk::IVector2<int> vec2(2, 3);
	auto minVec = spk::IVector2<int>::min(vec1, vec2);
	EXPECT_EQ(minVec, spk::IVector2<int>(1, 3)) << "Min method should return the correct component-wise minimum vector";
}

TEST_F(Vector2Test, MaxMethod)
{
	spk::IVector2<int> vec1(1, 4);
	spk::IVector2<int> vec2(2, 3);
	auto maxVec = spk::IVector2<int>::max(vec1, vec2);
	EXPECT_EQ(maxVec, spk::IVector2<int>(2, 4)) << "Max method should return the correct component-wise maximum vector";
}

TEST_F(Vector2Test, LerpMethod)
{
	spk::IVector2<int> start(0, 0);
	spk::IVector2<int> end(10, 10);
	auto lerpVec = spk::IVector2<int>::lerp(start, end, 0.5f);
	EXPECT_EQ(lerpVec, spk::IVector2<int>(5, 5)) << "Lerp method should return the correct linearly interpolated vector";
}
