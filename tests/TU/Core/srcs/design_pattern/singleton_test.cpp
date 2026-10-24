#include <design_pattern/singleton.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace
{
	template <typename TType>
	concept ValueInstanciable =
		requires(TType value) {
			spk::Singleton<TType>::instanciate(
				std::move(value));
		};

	template <typename TType>
	concept PointerInstanciable =
		requires(TType *value) {
			spk::Singleton<TType>::instanciate(value);
		};

	struct MissingValue
	{
	};

	struct MovableValue
	{
		int value = 0;
	};

	struct MoveOnlyValue
	{
		std::unique_ptr<int> value;

		explicit MoveOnlyValue(int p_value) :
			value(std::make_unique<int>(p_value))
		{
		}

		MoveOnlyValue(const MoveOnlyValue &) = delete;
		MoveOnlyValue &operator=(const MoveOnlyValue &) = delete;
		MoveOnlyValue(MoveOnlyValue &&) noexcept = default;
		MoveOnlyValue &operator=(MoveOnlyValue &&) noexcept = default;
	};

	struct PointerOnlyValue
	{
		int value = 0;

		explicit PointerOnlyValue(int p_value) :
			value(p_value)
		{
		}

		PointerOnlyValue(const PointerOnlyValue &) = delete;
		PointerOnlyValue(PointerOnlyValue &&) = delete;
	};

	struct MutableValue
	{
		int value = 1;
	};

	struct ReplaceableValue
	{
		int value = 0;
	};

	struct OwnedValue
	{
		std::shared_ptr<std::atomic<int>> destructions;

		explicit OwnedValue(
			std::shared_ptr<std::atomic<int>> p_destructions) :
			destructions(std::move(p_destructions))
		{
		}

		OwnedValue(const OwnedValue &) = delete;
		OwnedValue(OwnedValue &&) = delete;

		~OwnedValue()
		{
			destructions->fetch_add(
				1,
				std::memory_order_relaxed);
		}
	};

	static_assert(!std::is_constructible_v<
				  spk::Singleton<MovableValue>>);
	static_assert(ValueInstanciable<MovableValue>);
	static_assert(ValueInstanciable<MoveOnlyValue>);
	static_assert(!ValueInstanciable<PointerOnlyValue>);
	static_assert(PointerInstanciable<MovableValue>);
	static_assert(PointerInstanciable<PointerOnlyValue>);
}

TEST(Singleton, InstanceThrowsUntilInstanciated)
{
	EXPECT_FALSE(
		spk::Singleton<MissingValue>::isInstanciated());
	EXPECT_THROW(
		(void)spk::Singleton<MissingValue>::instance(),
		spk::Exception);
}

TEST(Singleton, InstanciatesMovableValue)
{
	spk::Singleton<MovableValue>::instanciate(
		MovableValue{37});

	EXPECT_TRUE(
		spk::Singleton<MovableValue>::isInstanciated());
	EXPECT_EQ(
		spk::Singleton<MovableValue>::instance().value,
		37);
}

TEST(Singleton, InstanciatesMoveOnlyValue)
{
	spk::Singleton<MoveOnlyValue>::instanciate(
		MoveOnlyValue(61));

	ASSERT_TRUE(
		spk::Singleton<MoveOnlyValue>::isInstanciated());
	ASSERT_NE(
		spk::Singleton<MoveOnlyValue>::instance().value,
		nullptr);
	EXPECT_EQ(
		*spk::Singleton<MoveOnlyValue>::instance().value,
		61);
}

TEST(Singleton, InstanciatesPointerForNonMovableType)
{
	spk::Singleton<PointerOnlyValue>::instanciate(
		new PointerOnlyValue(91));

	EXPECT_TRUE(
		spk::Singleton<PointerOnlyValue>::isInstanciated());
	EXPECT_EQ(
		spk::Singleton<PointerOnlyValue>::instance().value,
		91);
}

TEST(Singleton, InstanceReturnsMutableStableReference)
{
	spk::Singleton<MutableValue>::instanciate(
		MutableValue{});

	MutableValue &first =
		spk::Singleton<MutableValue>::instance();
	MutableValue &second =
		spk::Singleton<MutableValue>::instance();

	EXPECT_EQ(&first, &second);

	first.value = 73;
	EXPECT_EQ(second.value, 73);
}

TEST(Singleton, ReinstanciationReplacesPreviousValue)
{
	spk::Singleton<ReplaceableValue>::instanciate(
		ReplaceableValue{12});
	EXPECT_EQ(
		spk::Singleton<ReplaceableValue>::instance().value,
		12);

	spk::Singleton<ReplaceableValue>::instanciate(
		ReplaceableValue{44});
	EXPECT_EQ(
		spk::Singleton<ReplaceableValue>::instance().value,
		44);
}

TEST(Singleton, PointerReinstanciationDestroysPreviouslyOwnedValue)
{
	auto destructions =
		std::make_shared<std::atomic<int>>(0);

	spk::Singleton<OwnedValue>::instanciate(
		new OwnedValue(destructions));
	EXPECT_EQ(
		destructions->load(std::memory_order_relaxed),
		0);

	spk::Singleton<OwnedValue>::instanciate(
		new OwnedValue(destructions));
	EXPECT_EQ(
		destructions->load(std::memory_order_relaxed),
		1);
}

TEST(Singleton, RejectsNullPointer)
{
	EXPECT_THROW(
		spk::Singleton<PointerOnlyValue>::instanciate(
			nullptr),
		spk::Exception);
}

TEST(Singleton, NullPointerDoesNotReplaceExistingInstance)
{
	spk::Singleton<MutableValue>::instanciate(
		MutableValue{.value = 19});

	EXPECT_THROW(
		spk::Singleton<MutableValue>::instanciate(
			nullptr),
		spk::Exception);
	EXPECT_TRUE(
		spk::Singleton<MutableValue>::isInstanciated());
	EXPECT_EQ(
		spk::Singleton<MutableValue>::instance().value,
		19);
}
