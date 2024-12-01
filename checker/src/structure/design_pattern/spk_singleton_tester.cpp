#include "structure/design_pattern/spk_singleton_tester.hpp"

TEST_F(SingletonTest, SingletonInstanciation)
{
	{
		TestSingleton::Instanciator inst(42);
		EXPECT_EQ(inst->getValue(), 42);
		EXPECT_NE(TestSingleton::instance().get(), nullptr) << "Singleton instance should not be null after instantiation";
	}

	EXPECT_EQ(TestSingleton::instance().get(), nullptr) << "Singleton instance should be null after instanciator is out of scope";
}

TEST_F(SingletonTest, SingletonDoubleInstanciation)
{
	TestSingleton::Instanciator inst1(42);
	EXPECT_NO_THROW(TestSingleton::Instanciator inst2(43)) << "Second instantiation by Instanciator should no throw a runtime error";
	EXPECT_THROW(TestSingleton::instanciate(44), std::runtime_error) << "Second instantiation by instanciate should throw a runtime error";
}

TEST_F(SingletonTest, SingletonAccess)
{
	{
		TestSingleton::Instanciator inst(42);
		EXPECT_EQ(inst->getValue(), 42);

		auto instance = TestSingleton::instance();
		EXPECT_EQ(instance->getValue(), 42);
	}

	EXPECT_EQ(TestSingleton::instance().get(), nullptr) << "Singleton instance should be null after instanciator is out of scope";
}

TEST_F(SingletonTest, SingletonRelease)
{
	{
		TestSingleton::Instanciator inst(42);
		EXPECT_EQ(inst->getValue(), 42);
		EXPECT_NE(TestSingleton::instance().get(), nullptr) << "Singleton instance should not be null after instantiation";
	}

	TestSingleton::release();
	EXPECT_EQ(TestSingleton::instance().get(), nullptr) << "Singleton instance should be null after explicit release";
}

TEST_F(SingletonTest, SingletonConstAccess)
{
	{
		TestSingleton::Instanciator inst(42);
		EXPECT_EQ(inst->getValue(), 42);

		const auto& constInstance = TestSingleton::c_instance();
		EXPECT_EQ(constInstance->getValue(), 42);
	}

	EXPECT_EQ(TestSingleton::instance().get(), nullptr) << "Singleton instance should be null after instanciator is out of scope";
}