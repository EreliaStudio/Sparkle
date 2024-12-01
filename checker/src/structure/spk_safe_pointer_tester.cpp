#include "structure/spk_safe_pointer_tester.hpp"

TEST_F(SafePointerTest, Constructor)
{
	EXPECT_EQ(ptr->getValue(), 42);
}

TEST_F(SafePointerTest, DefaultConstructor)
{
	spk::SafePointer<spk::TestObject> defaultPtr;
	EXPECT_EQ(defaultPtr.get(), nullptr);
}

TEST_F(SafePointerTest, DereferenceOperator)
{
	EXPECT_EQ((*ptr).getValue(), 42);
	(*ptr).setValue(128);
	EXPECT_EQ((*ptr).getValue(), 128);
}

TEST_F(SafePointerTest, ArrowOperator)
{
	EXPECT_EQ(ptr->getValue(), 42);
	ptr->setValue(128);
	EXPECT_EQ(ptr->getValue(), 128);
}

TEST_F(SafePointerTest, GetMethod)
{
	EXPECT_EQ(ptr.get(), obj);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}