#include "structure/graphics/spk_geometry_2d_tester.hpp"

TEST_F(Geometry2DTest, DefaultConstructor)
{
	ASSERT_EQ(geometry.anchor.x, 0);
	ASSERT_EQ(geometry.anchor.y, 0);
	ASSERT_EQ(geometry.size.x, 0);
	ASSERT_EQ(geometry.size.y, 0);

	ASSERT_EQ(geometryInt.anchor.x, 0);
	ASSERT_EQ(geometryInt.anchor.y, 0);
	ASSERT_EQ(geometryInt.size.x, 0);
	ASSERT_EQ(geometryInt.size.y, 0);
}

TEST_F(Geometry2DTest, ParameterizedConstructor)
{
	spk::Geometry2D::Point anchor(10.5, 20.5);
	spk::Geometry2D::Size size(30.5, 40.5);
	spk::Geometry2D geometry(anchor, size);
	spk::Geometry2D geometry2(static_cast<int>(10.5), static_cast<int>(20.5), static_cast<size_t>(30.5), static_cast<size_t>(40.5));
	spk::Geometry2D geometry3{ 10, 20, 30, 40 };

	ASSERT_EQ(geometry.anchor.x, 10);
	ASSERT_EQ(geometry.anchor.y, 20);
	ASSERT_EQ(geometry.size.x, 30);
	ASSERT_EQ(geometry.size.y, 40);

	ASSERT_EQ(geometry2.anchor.x, 10);
	ASSERT_EQ(geometry2.anchor.y, 20);
	ASSERT_EQ(geometry2.size.x, 30);
	ASSERT_EQ(geometry2.size.y, 40);

	ASSERT_EQ(geometry3.anchor.x, 10);
	ASSERT_EQ(geometry3.anchor.y, 20);
	ASSERT_EQ(geometry3.size.x, 30);
	ASSERT_EQ(geometry3.size.y, 40);
}

TEST_F(Geometry2DTest, PositionAddition)
{
	spk::Geometry2D::Point pos1(10, 20);
	spk::Geometry2D::Point pos2(5, 15);

	auto result = pos1 + pos2;
	ASSERT_EQ(result.x, 15);
	ASSERT_EQ(result.y, 35);

	pos1 += pos2;
	ASSERT_EQ(pos1.x, 15);
	ASSERT_EQ(pos1.y, 35);
}

TEST_F(Geometry2DTest, PositionSubtraction)
{
	spk::Geometry2D::Point pos1(10, 20);
	spk::Geometry2D::Point pos2(5, 15);

	auto result = pos1 - pos2;
	ASSERT_EQ(result.x, 5);
	ASSERT_EQ(result.y, 5);

	pos1 -= pos2;
	ASSERT_EQ(pos1.x, 5);
	ASSERT_EQ(pos1.y, 5);
}

TEST_F(Geometry2DTest, PositionMultiplication)
{
	spk::Geometry2D::Point pos1(10, 20);
	spk::Geometry2D::Point pos2(5, 15);

	auto result = pos1 * pos2;
	ASSERT_EQ(result.x, 50);
	ASSERT_EQ(result.y, 300);

	pos1 *= pos2;
	ASSERT_EQ(pos1.x, 50);
	ASSERT_EQ(pos1.y, 300);
}

TEST_F(Geometry2DTest, PositionDivision)
{
	spk::Geometry2D::Point pos1(10, 20);
	spk::Geometry2D::Point pos2(2, 5);

	auto result = pos1 / pos2;
	ASSERT_EQ(result.x, 5);
	ASSERT_EQ(result.y, 4);

	pos1 /= pos2;
	ASSERT_EQ(pos1.x, 5);
	ASSERT_EQ(pos1.y, 4);

	ASSERT_THROW(pos1 / spk::Geometry2D::Point(0, 5), std::runtime_error);
	ASSERT_THROW(pos1 / spk::Geometry2D::Point(2, 0), std::runtime_error);
}

TEST_F(Geometry2DTest, SizeAddition)
{
	spk::Geometry2D::Size size1(10, 20);
	spk::Geometry2D::Size size2(5, 15);

	auto result = size1 + size2;
	ASSERT_EQ(result.x, 15);
	ASSERT_EQ(result.y, 35);

	size1 += size2;
	ASSERT_EQ(size1.x, 15);
	ASSERT_EQ(size1.y, 35);
}

TEST_F(Geometry2DTest, SizeSubtraction)
{
	spk::Geometry2D::Size size1(10, 20);
	spk::Geometry2D::Size size2(5, 15);

	auto result = size1 - size2;
	ASSERT_EQ(result.x, 5);
	ASSERT_EQ(result.y, 5);

	size1 -= size2;
	ASSERT_EQ(size1.x, 5);
	ASSERT_EQ(size1.y, 5);
}

TEST_F(Geometry2DTest, SizeMultiplication)
{
	spk::Geometry2D::Size size1(10, 20);
	spk::Geometry2D::Size size2(5, 15);

	auto result = size1 * size2;
	ASSERT_EQ(result.x, 50);
	ASSERT_EQ(result.y, 300);

	size1 *= size2;
	ASSERT_EQ(size1.x, 50);
	ASSERT_EQ(size1.y, 300);
}

TEST_F(Geometry2DTest, SizeDivision)
{
	spk::Geometry2D::Size size1(10, 20);
	spk::Geometry2D::Size size2(2, 5);

	auto result = size1 / size2;
	ASSERT_EQ(result.x, 5);
	ASSERT_EQ(result.y, 4);

	size1 /= size2;
	ASSERT_EQ(size1.x, 5);
	ASSERT_EQ(size1.y, 4);

	ASSERT_THROW(size1 / spk::Geometry2D::Size(0, 5), std::runtime_error);
	ASSERT_THROW(size1 / spk::Geometry2D::Size(2, 0), std::runtime_error);
}

TEST_F(Geometry2DTest, EqualityOperators)
{
	spk::Geometry2D::Point pos1(10, 20);
	spk::Geometry2D::Point pos2(10, 20);
	spk::Geometry2D::Point pos3(5, 15);

	ASSERT_TRUE(pos1 == pos2);
	ASSERT_FALSE(pos1 == pos3);
	ASSERT_TRUE(pos1 != pos3);
	ASSERT_FALSE(pos1 != pos2);

	spk::Geometry2D::Size size1(10, 20);
	spk::Geometry2D::Size size2(10, 20);
	spk::Geometry2D::Size size3(5, 15);

	ASSERT_TRUE(size1 == size2);
	ASSERT_FALSE(size1 == size3);
	ASSERT_TRUE(size1 != size3);
	ASSERT_FALSE(size1 != size2);
}

TEST_F(Geometry2DTest, GeometryEquality)
{
	spk::Geometry2D::Point anchor1(10, 20);
	spk::Geometry2D::Size size1(30, 40);
	spk::Geometry2D geometry1(anchor1, size1);

	spk::Geometry2D::Point anchor2(10, 20);
	spk::Geometry2D::Size size2(30, 40);
	spk::Geometry2D geometry2(anchor2, size2);

	spk::Geometry2D::Point anchor3(15, 25);
	spk::Geometry2D::Size size3(35, 45);
	spk::Geometry2D geometry3(anchor3, size3);

	// Check for equality
	ASSERT_TRUE(geometry1 == geometry2) << "geometry1 should be equal to geometry2";
	ASSERT_FALSE(geometry1 == geometry3) << "geometry1 should not be equal to geometry3";
}

TEST_F(Geometry2DTest, GeometryInequality)
{
	spk::Geometry2D::Point anchor1(10, 20);
	spk::Geometry2D::Size size1(30, 40);
	spk::Geometry2D geometry1(anchor1, size1);

	spk::Geometry2D::Point anchor2(10, 20);
	spk::Geometry2D::Size size2(30, 40);
	spk::Geometry2D geometry2(anchor2, size2);

	spk::Geometry2D::Point anchor3(15, 25);
	spk::Geometry2D::Size size3(35, 45);
	spk::Geometry2D geometry3(anchor3, size3);

	// Check for inequality
	ASSERT_FALSE(geometry1 != geometry2) << "geometry1 should be equal to geometry2, hence not unequal";
	ASSERT_TRUE(geometry1 != geometry3) << "geometry1 should be unequal to geometry3";
}

TEST_F(Geometry2DTest, OperatorStreamOutput)
{
	spk::Geometry2D::Point pos(10, 20);
	spk::Geometry2D::Size size(30, 40);
	spk::Geometry2D geometry(pos, size);

	std::wstringstream wss;
	wss << pos;
	std::wstring expectedPosOutput = L"(10, 20)";
	ASSERT_EQ(wss.str(), expectedPosOutput) << "Position output mismatch";

	wss.str(L""); // Clear the stream
	wss << size;
	std::wstring expectedSizeOutput = L"(30, 40)";
	ASSERT_EQ(wss.str(), expectedSizeOutput) << "Size output mismatch";

	wss.str(L""); // Clear the stream
	wss << geometry;
	std::wstring expectedGeometryOutput = L"Anchor : (10, 20) - Size : (30, 40)";
	ASSERT_EQ(wss.str(), expectedGeometryOutput) << "Geometry2D output mismatch";
}