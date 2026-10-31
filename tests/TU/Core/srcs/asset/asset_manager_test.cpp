#include <gtest/gtest.h>

#include "asset/manager.hpp"
#include "exception.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace asset_manager_test
{
	struct Tracked
	{
		inline static int constructions = 0;
		inline static int destructions = 0;

		std::string id;
		spk::JSON::Value config;
		int generation;

		Tracked(std::string name, spk::JSON::Value settings) :
			id(std::move(name)),
			config(std::move(settings)),
			generation(++constructions)
		{
		}

		~Tracked()
		{
			++destructions;
		}
	};

	struct Other
	{
		std::string id;
	};

	struct Immovable
	{
		int value;

		explicit Immovable(int initialValue) :
			value(initialValue)
		{
		}

		Immovable(const Immovable &) = delete;
		Immovable &operator=(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator=(Immovable &&) = delete;
	};

	struct Dependency
	{
		inline static int constructions = 0;
		std::string id;

		explicit Dependency(std::string name) :
			id(std::move(name))
		{
			++constructions;
		}
	};

	struct Parent
	{
		inline static bool fail = false;
		Dependency *dependency;
	};

	struct Recursive
	{
		int depth;
	};

	struct Flaky
	{
		inline static bool failNext = false;
		inline static int attempts = 0;

		int attempt;
	};

	struct ConstructorFailure
	{
		inline static bool fail = false;
		inline static int constructions = 0;
		inline static int destructions = 0;

		ConstructorFailure()
		{
			++constructions;
			if (fail)
			{
				throw std::runtime_error("construction failed");
			}
		}

		~ConstructorFailure()
		{
			++destructions;
		}
	};
}

namespace spk::asset
{
	template <>
	struct Factory<asset_manager_test::Tracked>
	{
		static asset_manager_test::Tracked create(Manager &, const std::string &id, const JSON::Value &config)
		{
			return asset_manager_test::Tracked(id, config);
		}
	};

	template <>
	struct Factory<asset_manager_test::Other>
	{
		static asset_manager_test::Other create(Manager &, const std::string &id, const JSON::Value &)
		{
			return asset_manager_test::Other{id};
		}
	};

	template <>
	struct Factory<asset_manager_test::Immovable>
	{
		static asset_manager_test::Immovable create(Manager &, const std::string &, const JSON::Value &config)
		{
			return asset_manager_test::Immovable(config.at("value").as<int>());
		}
	};

	template <>
	struct Factory<asset_manager_test::Dependency>
	{
		static asset_manager_test::Dependency create(Manager &, const std::string &id, const JSON::Value &)
		{
			return asset_manager_test::Dependency(id);
		}
	};

	template <>
	struct Factory<asset_manager_test::Parent>
	{
		static asset_manager_test::Parent create(Manager &assets, const std::string &, const JSON::Value &config)
		{
			auto &dependency = assets.load<asset_manager_test::Dependency>("shared", config);
			if (asset_manager_test::Parent::fail)
			{
				throw std::runtime_error("parent failed");
			}
			return asset_manager_test::Parent{&dependency};
		}
	};

	template <>
	struct Factory<asset_manager_test::Recursive>
	{
		static asset_manager_test::Recursive create(Manager &assets, const std::string &id, const JSON::Value &config)
		{
			if (id == "direct")
			{
				(void)assets.load<asset_manager_test::Recursive>(id, config);
			}
			else if (id == "A" || id == "B")
			{
				(void)assets.load<asset_manager_test::Recursive>(id == "A" ? "B" : "A", config);
			}
			else if (id == "recursive")
			{
				const int depth = config.at("depth").as<int>();
				if (depth > 0)
				{
					JSON::Value child = config;
					child["depth"] = depth - 1;
					(void)assets.load<asset_manager_test::Recursive>(id, child);
				}
				return asset_manager_test::Recursive{depth};
			}
			return asset_manager_test::Recursive{0};
		}
	};

	template <>
	struct Factory<asset_manager_test::Flaky>
	{
		static asset_manager_test::Flaky create(Manager &, const std::string &, const JSON::Value &)
		{
			const int attempt = ++asset_manager_test::Flaky::attempts;
			if (asset_manager_test::Flaky::failNext)
			{
				asset_manager_test::Flaky::failNext = false;
				throw std::runtime_error("factory failed");
			}
			return asset_manager_test::Flaky{attempt};
		}
	};

	template <>
	struct Factory<asset_manager_test::ConstructorFailure>
	{
		static asset_manager_test::ConstructorFailure create(Manager &, const std::string &, const JSON::Value &)
		{
			return asset_manager_test::ConstructorFailure();
		}
	};
}

namespace
{
	[[nodiscard]] spk::JSON::Value grid(int width, int height)
	{
		spk::JSON::Value result = spk::JSON::Value::object();
		result["size"] = spk::JSON::Value::object();
		result["size"]["x"] = width;
		result["size"]["y"] = height;
		return result;
	}

	[[nodiscard]] bool contains(std::string_view text, std::string_view fragment)
	{
		return text.find(fragment) != std::string_view::npos;
	}
}

TEST(AssetManagerTest, RepeatedIdentityReusesOneInstanceAndFactoryCall)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	const auto config = grid(16, 16);

	auto &first = assets.load<asset_manager_test::Tracked>("terrain", config);
	auto &second = assets.load<asset_manager_test::Tracked>("terrain", config);
	auto &third = assets.load<asset_manager_test::Tracked>("terrain", config);

	EXPECT_EQ(&first, &second);
	EXPECT_EQ(&second, &third);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 1);
	EXPECT_EQ(first.id, "terrain");
	EXPECT_EQ(first.config, config);
}

TEST(AssetManagerTest, DistinctGridConfigurationsHaveDistinctIdentities)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;

	auto &sixteen = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	auto &eight = assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8));

	EXPECT_NE(&sixteen, &eight);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 2);
	EXPECT_EQ(sixteen.config, grid(16, 16));
	EXPECT_EQ(eight.config, grid(8, 8));
}

TEST(AssetManagerTest, JsonObjectPropertyOrderDoesNotChangeIdentity)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	const auto firstConfig = spk::JSON::Value::fromString(R"({"grid":{"x":16,"y":8},"role":"albedo"})");
	const auto secondConfig = spk::JSON::Value::fromString(R"({"role":"albedo","grid":{"y":8,"x":16}})");

	EXPECT_EQ(
		&assets.load<asset_manager_test::Tracked>("atlas", firstConfig),
		&assets.load<asset_manager_test::Tracked>("atlas", secondConfig));
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 1);
}

TEST(AssetManagerTest, DefaultConfigurationIsEmptyObjectRatherThanNull)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto &defaultAsset = assets.load<asset_manager_test::Tracked>("id");
	auto &explicitObject = assets.load<asset_manager_test::Tracked>("id", spk::JSON::Value::object());
	auto &nullAsset = assets.load<asset_manager_test::Tracked>("id", spk::JSON::Value{});

	EXPECT_EQ(&defaultAsset, &explicitObject);
	EXPECT_NE(&defaultAsset, &nullAsset);
	EXPECT_TRUE(defaultAsset.config.isObject());
	EXPECT_TRUE(nullAsset.config.isNull());
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 2);
}

TEST(AssetManagerTest, DifferentCppTypesShareIdentifierWithoutSharingAsset)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("shared", grid(16, 16));
	auto &second = assets.load<asset_manager_test::Other>("shared", grid(16, 16));

	EXPECT_NE(static_cast<void *>(&first), static_cast<void *>(&second));
	EXPECT_EQ(second.id, "shared");
	EXPECT_EQ(&first, &assets.load<asset_manager_test::Tracked>("shared", grid(16, 16)));
}

TEST(AssetManagerTest, NonMovableNonCopyableAssetIsConstructedInPlace)
{
	static_assert(!std::is_copy_constructible_v<asset_manager_test::Immovable>);
	static_assert(!std::is_move_constructible_v<asset_manager_test::Immovable>);

	spk::asset::Manager assets;
	const auto config = spk::JSON::Value::fromString(R"({"value":42})");

	auto &first = assets.load<asset_manager_test::Immovable>("immovable", config);
	auto &second = assets.load<asset_manager_test::Immovable>("immovable", config);

	EXPECT_EQ(&first, &second);
	EXPECT_EQ(first.value, 42);
}

TEST(AssetManagerTest, FurtherInsertionsPreserveExistingAddresses)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("first");
	auto &second = assets.load<asset_manager_test::Tracked>("second");
	const auto *firstAddress = &first;
	const auto *secondAddress = &second;

	for (int index = 0; index < 128; ++index)
	{
		(void)assets.load<asset_manager_test::Tracked>("generated-" + std::to_string(index), grid(index, 1));
	}

	EXPECT_EQ(&first, firstAddress);
	EXPECT_EQ(&second, secondAddress);
	EXPECT_EQ(&assets.load<asset_manager_test::Tracked>("first"), firstAddress);
}

TEST(AssetManagerTest, NestedDependenciesAreReusedAcrossParents)
{
	asset_manager_test::Dependency::constructions = 0;
	asset_manager_test::Parent::fail = false;
	spk::asset::Manager assets;
	const auto config = grid(16, 16);

	auto &first = assets.load<asset_manager_test::Parent>("parent-a", config);
	auto &second = assets.load<asset_manager_test::Parent>("parent-b", config);
	auto &dependency = assets.load<asset_manager_test::Dependency>("shared", config);

	EXPECT_EQ(first.dependency, &dependency);
	EXPECT_EQ(second.dependency, &dependency);
	EXPECT_EQ(asset_manager_test::Dependency::constructions, 1);
}

TEST(AssetManagerTest, DirectCycleProducesDescriptiveSparkleException)
{
	spk::asset::Manager assets;
	try
	{
		(void)assets.load<asset_manager_test::Recursive>("direct");
		FAIL() << "Expected a dependency cycle";
	} catch (const spk::Exception &exception)
	{
		EXPECT_TRUE(contains(exception.message(), "cycle"));
		EXPECT_TRUE(contains(exception.message(), "direct"));
		EXPECT_TRUE(contains(exception.message(), "{}"));
	}
}

TEST(AssetManagerTest, IndirectCycleReportsDependencyChain)
{
	spk::asset::Manager assets;
	try
	{
		(void)assets.load<asset_manager_test::Recursive>("A");
		FAIL() << "Expected a dependency cycle";
	} catch (const spk::Exception &exception)
	{
		EXPECT_TRUE(contains(exception.message(), "A"));
		EXPECT_TRUE(contains(exception.message(), "B"));
		EXPECT_TRUE(contains(exception.message(), "->"));
	}
}

TEST(AssetManagerTest, SameIdentifierWithDifferentActiveConfigurationsIsNotACycle)
{
	spk::asset::Manager assets;
	auto &parent = assets.load<asset_manager_test::Recursive>(
		"recursive", spk::JSON::Value::fromString(R"({"depth":4})"));
	auto &child = assets.load<asset_manager_test::Recursive>(
		"recursive", spk::JSON::Value::fromString(R"({"depth":3})"));

	EXPECT_EQ(parent.depth, 4);
	EXPECT_EQ(child.depth, 3);
	EXPECT_NE(&parent, &child);
}

TEST(AssetManagerTest, FactoryFailureCanRetryWithoutPublishingIncompleteAsset)
{
	asset_manager_test::Flaky::attempts = 0;
	asset_manager_test::Flaky::failNext = true;
	spk::asset::Manager assets;

	EXPECT_THROW((void)assets.load<asset_manager_test::Flaky>("unstable"), std::runtime_error);
	auto &successful = assets.load<asset_manager_test::Flaky>("unstable");
	auto &same = assets.load<asset_manager_test::Flaky>("unstable");

	EXPECT_EQ(successful.attempt, 2);
	EXPECT_EQ(&successful, &same);
	EXPECT_EQ(asset_manager_test::Flaky::attempts, 2);
}

TEST(AssetManagerTest, PreviouslyLoadedDependenciesSurviveFailedParent)
{
	asset_manager_test::Dependency::constructions = 0;
	asset_manager_test::Parent::fail = true;
	spk::asset::Manager assets;
	const auto config = grid(8, 8);

	EXPECT_THROW((void)assets.load<asset_manager_test::Parent>("parent", config), std::runtime_error);
	auto &dependency = assets.load<asset_manager_test::Dependency>("shared", config);
	EXPECT_EQ(asset_manager_test::Dependency::constructions, 1);

	asset_manager_test::Parent::fail = false;
	auto &parent = assets.load<asset_manager_test::Parent>("parent", config);
	EXPECT_EQ(parent.dependency, &dependency);
	EXPECT_EQ(asset_manager_test::Dependency::constructions, 1);
}

TEST(AssetManagerTest, ConstructorExceptionDoesNotPublishAsset)
{
	asset_manager_test::ConstructorFailure::constructions = 0;
	asset_manager_test::ConstructorFailure::destructions = 0;
	asset_manager_test::ConstructorFailure::fail = true;
	spk::asset::Manager assets;

	EXPECT_THROW((void)assets.load<asset_manager_test::ConstructorFailure>("constructor"), std::runtime_error);
	asset_manager_test::ConstructorFailure::fail = false;
	auto &instance = assets.load<asset_manager_test::ConstructorFailure>("constructor");
	(void)instance;

	EXPECT_EQ(asset_manager_test::ConstructorFailure::constructions, 2);
	EXPECT_EQ(asset_manager_test::ConstructorFailure::destructions, 0);
	assets.clear();
	EXPECT_EQ(asset_manager_test::ConstructorFailure::destructions, 1);
}

TEST(AssetManagerTest, FailedCycleClearsActiveStackForSubsequentLoads)
{
	spk::asset::Manager assets;
	EXPECT_THROW((void)assets.load<asset_manager_test::Recursive>("A"), spk::Exception);
	auto &safe = assets.load<asset_manager_test::Recursive>("safe");
	EXPECT_EQ(safe.depth, 0);
	EXPECT_EQ(&safe, &assets.load<asset_manager_test::Recursive>("safe"));
}

TEST(AssetManagerTest, ClearDestroysOwnedAssetsAndPermitsReuse)
{
	asset_manager_test::Tracked::constructions = 0;
	asset_manager_test::Tracked::destructions = 0;
	spk::asset::Manager assets;
	(void)assets.load<asset_manager_test::Tracked>("first");
	(void)assets.load<asset_manager_test::Tracked>("second");

	EXPECT_EQ(asset_manager_test::Tracked::destructions, 0);
	assets.clear();
	EXPECT_EQ(asset_manager_test::Tracked::destructions, 2);
	auto &newAsset = assets.load<asset_manager_test::Tracked>("first");
	EXPECT_EQ(newAsset.generation, 3);
	EXPECT_EQ(&newAsset, &assets.load<asset_manager_test::Tracked>("first"));
	assets.clear();
	EXPECT_EQ(asset_manager_test::Tracked::destructions, 3);
}

TEST(AssetManagerTest, StoredIdentityDoesNotBorrowConfigurationArgument)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto config = grid(16, 16);
	auto &original = assets.load<asset_manager_test::Tracked>("terrain", config);
	config["size"]["x"] = 8;

	EXPECT_EQ(&original, &assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16)));
	EXPECT_NE(&original, &assets.load<asset_manager_test::Tracked>("terrain", config));
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 2);
}

TEST(AssetManagerTest, ArrayOrderAndJsonTypesRemainDistinct)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("value", spk::JSON::Value::fromString("[1,2]"));
	auto &second = assets.load<asset_manager_test::Tracked>("value", spk::JSON::Value::fromString("[2,1]"));
	auto &integer = assets.load<asset_manager_test::Tracked>("number", spk::JSON::Value(1));
	auto &floating = assets.load<asset_manager_test::Tracked>("number", spk::JSON::Value(1.0));

	EXPECT_NE(&first, &second);
	EXPECT_NE(&integer, &floating);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 4);
}

TEST(AssetManagerTest, IdentifierIsNotNormalizedAsAPath)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Other>("atlas/../terrain.png");
	auto &second = assets.load<asset_manager_test::Other>("terrain.png");
	EXPECT_NE(&first, &second);
}

TEST(AssetManagerTest, GetReturnsPreviouslyLoadedAssetByNameWithoutFactoryCall)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto &created = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));

	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &created);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 1);
}

TEST(AssetManagerTest, GetThrowsForUnknownNameOrType)
{
	spk::asset::Manager assets;
	(void)assets.load<asset_manager_test::Tracked>("terrain");

	EXPECT_THROW((void)assets.get<asset_manager_test::Tracked>("missing"), spk::Exception);
	EXPECT_THROW((void)assets.get<asset_manager_test::Other>("terrain"), spk::Exception);
}

TEST(AssetManagerTest, GetSelectsFirstLoadedConfigurationByDefault)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	(void)assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8));
	auto &other = assets.load<asset_manager_test::Other>("terrain");

	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &first);
	EXPECT_EQ(&assets.get<asset_manager_test::Other>("terrain"), &other);
}

TEST(AssetManagerTest, GetAfterClearCannotReturnAnExpiredAsset)
{
	spk::asset::Manager assets;
	(void)assets.load<asset_manager_test::Tracked>("terrain");
	assets.clear();

	EXPECT_THROW((void)assets.get<asset_manager_test::Tracked>("terrain"), spk::Exception);
	auto &newAsset = assets.load<asset_manager_test::Tracked>("terrain");
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &newAsset);
}

TEST(AssetManagerTest, LoadCanSelectNewConfigurationAsDefault)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	auto &second = assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8), true);

	EXPECT_NE(&first, &second);
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &second);
}

TEST(AssetManagerTest, ReloadingAnExistingConfigurationCanSelectItAsDefault)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	(void)assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8), true);

	EXPECT_EQ(&assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16), true), &first);
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &first);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 2);
}

TEST(AssetManagerTest, SetDefaultSwitchesAmongAlreadyLoadedConfigurations)
{
	asset_manager_test::Tracked::constructions = 0;
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	auto &second = assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8));

	assets.setDefault<asset_manager_test::Tracked>("terrain", grid(8, 8));
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &second);
	assets.setDefault<asset_manager_test::Tracked>("terrain", grid(16, 16));
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &first);
	EXPECT_EQ(asset_manager_test::Tracked::constructions, 2);
}

TEST(AssetManagerTest, SetDefaultRejectsMissingConfigurationWithoutChangingDefault)
{
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	EXPECT_THROW(assets.setDefault<asset_manager_test::Tracked>("terrain", grid(8, 8)), spk::Exception);
	EXPECT_THROW(assets.setDefault<asset_manager_test::Tracked>("unknown", grid(16, 16)), spk::Exception);
	EXPECT_THROW(assets.setDefault<asset_manager_test::Other>("terrain"), spk::Exception);
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &first);
}

TEST(AssetManagerTest, FailedFactoryCannotChangeExistingDefault)
{
	asset_manager_test::Flaky::failNext = false;
	spk::asset::Manager assets;
	auto &first = assets.load<asset_manager_test::Flaky>("unstable");
	asset_manager_test::Flaky::failNext = true;

	EXPECT_THROW(
		(void)assets.load<asset_manager_test::Flaky>("unstable", grid(8, 8), true),
		std::runtime_error);
	EXPECT_EQ(&assets.get<asset_manager_test::Flaky>("unstable"), &first);
}

TEST(AssetManagerTest, DefaultIsTrackedIndependentlyForEachTypeAndName)
{
	spk::asset::Manager assets;
	auto &terrain = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	auto &other = assets.load<asset_manager_test::Other>("terrain");
	auto &water = assets.load<asset_manager_test::Tracked>("water");
	(void)assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8), true);

	EXPECT_EQ(&assets.get<asset_manager_test::Other>("terrain"), &other);
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("water"), &water);
	EXPECT_NE(&assets.get<asset_manager_test::Tracked>("terrain"), &terrain);
}

TEST(AssetManagerTest, SetDefaultWithoutConfigSelectsExplicitEmptyObject)
{
	spk::asset::Manager assets;
	(void)assets.load<asset_manager_test::Tracked>("terrain", spk::JSON::Value{});
	auto &emptyObject = assets.load<asset_manager_test::Tracked>("terrain");

	assets.setDefault<asset_manager_test::Tracked>("terrain");
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &emptyObject);
}

TEST(AssetManagerTest, ClearResetsDefaultSelection)
{
	spk::asset::Manager assets;
	(void)assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	(void)assets.load<asset_manager_test::Tracked>("terrain", grid(8, 8), true);
	assets.clear();

	EXPECT_THROW((void)assets.get<asset_manager_test::Tracked>("terrain"), spk::Exception);
	auto &fresh = assets.load<asset_manager_test::Tracked>("terrain", grid(16, 16));
	EXPECT_EQ(&assets.get<asset_manager_test::Tracked>("terrain"), &fresh);
}
