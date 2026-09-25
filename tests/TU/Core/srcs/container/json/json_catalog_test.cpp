#include <container/json/catalog.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>
#include <type/uuid.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	class TemporaryJsonFile
	{
	private:
		std::filesystem::path _path;

	public:
		explicit TemporaryJsonFile(std::string content) :
			_path(
				std::filesystem::temp_directory_path() /
				("sparkle-json-catalog-" + spk::UUID::generate().toString() + ".json"))
		{
			std::ofstream stream(_path, std::ios::binary);
			stream << content;
		}

		TemporaryJsonFile(const TemporaryJsonFile &) = delete;
		TemporaryJsonFile &operator=(const TemporaryJsonFile &) = delete;

		~TemporaryJsonFile()
		{
			std::error_code error;
			std::filesystem::remove(_path, error);
		}

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}
	};

	struct Element
	{
		using ID = std::uint32_t;

		int value = 0;

		explicit Element(int p_value) :
			value(p_value)
		{
		}

		Element(const Element &) = delete;
		Element &operator=(const Element &) = delete;
		Element(Element &&) noexcept = default;
		Element &operator=(Element &&) noexcept = delete;
	};

	static_assert(std::is_move_constructible_v<Element>);
	static_assert(!std::is_copy_constructible_v<Element>);

	class Catalog final : public spk::JSON::Catalog<Element>
	{
	private:
		[[nodiscard]] ID _parseKey(const spk::JSON::Reader &reader) const override
		{
			return reader.require<ID>("id");
		}

		[[nodiscard]] Element _parseElement(const spk::JSON::Reader &reader) const override
		{
			reader.forbidUnknown({"value"});
			return Element(reader.require<int>("value"));
		}

	public:
		void insert(ID id, Element element)
		{
			_insert(id, std::move(element));
		}
	};
}

TEST(JsonCatalog, LoadsMoveOnlyElementsAndExposesLookupApi)
{
	const TemporaryJsonFile file(
		R"({"elements":[{"id":1,"data":{"value":10}},{"id":2,"data":{"value":20}}]})");

	Catalog catalog;
	catalog.load(file.path());

	EXPECT_TRUE(catalog.contains(1u));
	EXPECT_TRUE(catalog.contains(2u));
	EXPECT_EQ(catalog.at(1u).value, 10);
	EXPECT_EQ(catalog[2u].value, 20);
	EXPECT_EQ(catalog.tryGet(1u), &catalog.at(1u));
	EXPECT_EQ(catalog.tryGet(99u), nullptr);
	EXPECT_THROW((void)catalog.at(99u), spk::Exception);
	EXPECT_THROW((void)catalog[99u], spk::Exception);
}

TEST(JsonCatalog, LoadsDirectSingleElementRootThroughSameParser)
{
	const TemporaryJsonFile file(
		R"({"id":3,"data":{"value":30}})");

	Catalog catalog;
	catalog.load(file.path());

	EXPECT_TRUE(catalog.contains(3u));
	EXPECT_EQ(catalog.at(3u).value, 30);
}

TEST(JsonCatalog, AggregateAndDirectLoadsRejectDuplicateIdsConsistently)
{
	const TemporaryJsonFile aggregate(
		R"({"elements":[{"id":4,"data":{"value":40}}]})");
	const TemporaryJsonFile direct(
		R"({"id":4,"data":{"value":41}})");

	Catalog catalog;
	catalog.load(aggregate.path());

	EXPECT_THROW(catalog.load(direct.path()), spk::Exception);
	EXPECT_EQ(catalog.at(4u).value, 40);
}

TEST(JsonCatalog, ProtectedInsertUsesCatalogStorageAndRejectsDuplicate)
{
	Catalog catalog;
	catalog.insert(7u, Element(70));

	EXPECT_EQ(catalog.at(7u).value, 70);
	EXPECT_THROW(catalog.insert(7u, Element(71)), spk::Exception);
	EXPECT_EQ(catalog.at(7u).value, 70);
}

TEST(JsonCatalog, ElementReferencesRemainStableAcrossInsertions)
{
	Catalog catalog;
	catalog.insert(1u, Element(10));
	const Element *first = &catalog.at(1u);

	for (std::uint32_t id = 2u; id <= 1024u; ++id)
	{
		catalog.insert(id, Element(static_cast<int>(id)));
	}

	EXPECT_EQ(&catalog.at(1u), first);
	EXPECT_EQ(first->value, 10);
}

TEST(JsonCatalog, DuplicateWithinLoadPreservesEarlierElement)
{
	const TemporaryJsonFile file(
		R"({"elements":[{"id":1,"data":{"value":10}},{"id":1,"data":{"value":11}},{"id":2,"data":{"value":20}}]})");

	Catalog catalog;
	EXPECT_THROW(catalog.load(file.path()), spk::Exception);

	EXPECT_TRUE(catalog.contains(1u));
	EXPECT_EQ(catalog.at(1u).value, 10);
	EXPECT_FALSE(catalog.contains(2u));
}

TEST(JsonCatalog, DuplicateAgainstPreviousLoadThrows)
{
	const TemporaryJsonFile first(R"({"elements":[{"id":1,"data":{"value":10}}]})");
	const TemporaryJsonFile second(R"({"elements":[{"id":1,"data":{"value":20}}]})");

	Catalog catalog;
	catalog.load(first.path());

	EXPECT_THROW(catalog.load(second.path()), spk::Exception);
	EXPECT_EQ(catalog.at(1u).value, 10);
}

TEST(JsonCatalog, ElementParseFailureIsIncrementalAndStopsIteration)
{
	const TemporaryJsonFile file(
		R"({"elements":[{"id":1,"data":{"value":10}},{"id":2,"data":{}},{"id":3,"data":{"value":30}}]})");

	Catalog catalog;
	EXPECT_THROW(catalog.load(file.path()), spk::Exception);

	EXPECT_TRUE(catalog.contains(1u));
	EXPECT_FALSE(catalog.contains(2u));
	EXPECT_FALSE(catalog.contains(3u));
}

TEST(JsonCatalog, RejectsMalformedEnvelopeWithSourceContext)
{
	const std::string fixtures[] = {
		R"({})",
		R"({"elements":{}})",
		R"({"elements":[7]})",
		R"({"elements":[{"data":{"value":1}}]})",
		R"({"elements":[{"id":1}]})",
		R"({"elements":[{"id":1,"data":{"value":1},"unexpected":true}]})",
		R"({"elements":[],"unexpected":true})",
		R"({"id":1})",
		R"({"id":1,"data":{"value":1},"unexpected":true})"};

	for (const std::string &fixture : fixtures)
	{
		const TemporaryJsonFile file(fixture);
		Catalog catalog;

		try
		{
			catalog.load(file.path());
			FAIL() << "Expected malformed catalog to throw";
		} catch (const spk::Exception &exception)
		{
			const std::string message = exception.what();
			EXPECT_NE(message.find(file.path().generic_string()), std::string::npos);
			EXPECT_NE(message.find('$'), std::string::npos);
		}
	}
}
