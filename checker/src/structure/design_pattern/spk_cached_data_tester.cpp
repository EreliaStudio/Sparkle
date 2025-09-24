#include "structure/design_pattern/spk_cached_data_tester.hpp"

#include <string>

TEST_F(CachedDataTest, LazyInitialization)
{
	ASSERT_EQ(generatorCalls, 0);

	int &value = intCache.get();
	ASSERT_EQ(value, 10);
	ASSERT_EQ(generatorCalls, 1);

	int &secondValue = intCache.get();
	ASSERT_EQ(&value, &secondValue);
	ASSERT_EQ(generatorCalls, 1);
}

TEST_F(CachedDataTest, DereferenceOperator)
{
	int &value = *intCache;
	ASSERT_EQ(value, 10);
	ASSERT_EQ(generatorCalls, 1);

	int &direct = intCache.get();
	ASSERT_EQ(&value, &direct);
	ASSERT_EQ(generatorCalls, 1);
}

TEST_F(CachedDataTest, ReleaseForcesRegeneration)
{
	int &value = intCache.get();
	ASSERT_EQ(value, 10);
	ASSERT_EQ(generatorCalls, 1);

	intCache.release();
	ASSERT_EQ(generatorCalls, 1);

	int &afterRelease = intCache.get();
	ASSERT_EQ(afterRelease, 20);
	ASSERT_EQ(generatorCalls, 2);
}

TEST_F(CachedDataTest, IsCachedReflectsState)
{
	ASSERT_EQ(intCache.isCached(), true);

	(void)intCache.get();
	ASSERT_EQ(intCache.isCached(), false);

	intCache.release();
	ASSERT_EQ(intCache.isCached(), true);
}

TEST_F(CachedDataTest, WorksWithComplexTypes)
{
	int callCounter = 0;
	spk::CachedData<std::string> stringCache(
		[&callCounter]()
		{
			callCounter++;
			return std::string("value_" + std::to_string(callCounter));
		});

	ASSERT_EQ(callCounter, 0);
	const std::string &first = stringCache.get();
	ASSERT_EQ(first, "value_1");
	ASSERT_EQ(callCounter, 1);

	const std::string &second = stringCache.get();
	ASSERT_EQ(&first, &second);
	ASSERT_EQ(callCounter, 1);

	stringCache.release();
	const std::string &afterRelease = stringCache.get();
	ASSERT_EQ(afterRelease, "value_2");
	ASSERT_EQ(callCounter, 2);
}
