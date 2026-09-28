#include <system/translator.hpp>

#include <diagnostics/logger.hpp>
#include <exception.hpp>
#include <gtest/gtest.h>
#include <type/uuid.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	class TemporaryTranslationFile final
	{
	private:
		std::filesystem::path _path;

	public:
		explicit TemporaryTranslationFile(
			const std::string &content)
		{
			_path =
				std::filesystem::temp_directory_path() /
				("sparkle-translator-" +
				 spk::UUID::generate().toString() +
				 ".json");

			std::ofstream stream(_path);
			stream << content;
		}

		~TemporaryTranslationFile()
		{
			std::error_code error;
			std::filesystem::remove(_path, error);
		}

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}
	};
}

TEST(TranslatorTest, DirectAppendStoresTranslation)
{
	spk::Translator translator;
	translator.append("message", "Hello");

	EXPECT_EQ(
		translator.translate("message"),
		"Hello");
}

TEST(TranslatorTest, DirectAppendSupportsEmptyTranslation)
{
	spk::Translator translator;
	translator.append("message", "");

	EXPECT_EQ(
		translator.translate("message"),
		"");
}

TEST(TranslatorTest, DirectAppendSupportsDottedKeys)
{
	spk::Translator translator;
	translator.append(
		"client.connection.maximum_attempts_reached",
		"Stopped");

	EXPECT_EQ(
		translator.translate(
			"client.connection.maximum_attempts_reached"),
		"Stopped");
}

TEST(TranslatorTest, DirectAppendRejectsDuplicateKey)
{
	spk::Translator translator;
	translator.append("message", "First");

	EXPECT_THROW(
		translator.append("message", "Second"),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("message"),
		"First");
}

TEST(TranslatorTest, FormatsSingleArgument)
{
	spk::Translator translator;
	translator.append(
		"connection.failed",
		"Unable to connect after {} attempts");

	EXPECT_EQ(
		translator.translate("connection.failed", 5),
		"Unable to connect after 5 attempts");
}

TEST(TranslatorTest, FormatsMultipleArguments)
{
	spk::Translator translator;
	translator.append(
		"connection.endpoint",
		"Connecting to {}:{} on attempt {}");

	EXPECT_EQ(
		translator.translate(
			"connection.endpoint",
			"127.0.0.1",
			2550,
			3),
		"Connecting to 127.0.0.1:2550 on attempt 3");
}

TEST(TranslatorTest, SupportsPositionalArguments)
{
	spk::Translator translator;
	translator.append(
		"ordered",
		"{1} then {0}");

	EXPECT_EQ(
		translator.translate(
			"ordered",
			"first",
			"second"),
		"second then first");
}

TEST(TranslatorTest, SupportsRepeatedPositionalArguments)
{
	spk::Translator translator;
	translator.append(
		"repeated",
		"{0} / {0} / {1}");

	EXPECT_EQ(
		translator.translate(
			"repeated",
			"A",
			"B"),
		"A / A / B");
}

TEST(TranslatorTest, SupportsFormatSpecifiers)
{
	spk::Translator translator;
	translator.append(
		"padded",
		"{:04}");

	EXPECT_EQ(
		translator.translate("padded", 7),
		"0007");
}

TEST(TranslatorTest, SupportsEscapedBraces)
{
	spk::Translator translator;
	translator.append(
		"braces",
		"{{value}} = {}");

	EXPECT_EQ(
		translator.translate("braces", 42),
		"{value} = 42");
}

TEST(TranslatorTest, JsonAppendLoadsEveryTranslation)
{
	const TemporaryTranslationFile file(
		R"({"first":"First","second":"Second {}","ordered":"{1} then {0}"})");

	spk::Translator translator;
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("first"),
		"First");
	EXPECT_EQ(
		translator.translate("second", 2),
		"Second 2");
	EXPECT_EQ(
		translator.translate(
			"ordered",
			"first",
			"second"),
		"second then first");
}

TEST(TranslatorTest, JsonAppendPreservesExistingUnrelatedTranslations)
{
	const TemporaryTranslationFile file(
		R"({"from.file":"File"})");

	spk::Translator translator;
	translator.append("existing", "Existing");
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
	EXPECT_EQ(
		translator.translate("from.file"),
		"File");
}

TEST(TranslatorTest, JsonAppendRejectsDuplicateExistingKeyWithoutPartialAppend)
{
	const TemporaryTranslationFile file(
		R"({"shared":"File","new":"New"})");

	spk::Translator translator;
	translator.append("shared", "Initial");
	translator.append("preserved", "Preserved");

	EXPECT_THROW(
		translator.append(file.path()),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("shared"),
		"Initial");
	EXPECT_EQ(
		translator.translate("preserved"),
		"Preserved");
	EXPECT_EQ(
		translator.translate("new"),
		"new");
}

TEST(TranslatorTest, EmptyJsonCatalogDoesNotChangeExistingTranslations)
{
	const TemporaryTranslationFile file(R"({})");

	spk::Translator translator;
	translator.append("existing", "Existing");
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
}

TEST(TranslatorTest, MultipleJsonCatalogsAccumulateDistinctTranslations)
{
	const TemporaryTranslationFile first(
		R"({"first":"First","shared":"Shared"})");
	const TemporaryTranslationFile second(
		R"({"second":"Second"})");

	spk::Translator translator;
	translator.append(first.path());
	translator.append(second.path());

	EXPECT_EQ(
		translator.translate("first"),
		"First");
	EXPECT_EQ(
		translator.translate("second"),
		"Second");
	EXPECT_EQ(
		translator.translate("shared"),
		"Shared");
}

TEST(TranslatorTest, MultipleJsonCatalogsRejectDuplicateKeysTransactionally)
{
	const TemporaryTranslationFile first(
		R"({"first":"First","shared":"Shared"})");
	const TemporaryTranslationFile second(
		R"({"second":"Second","shared":"Duplicate"})");

	spk::Translator translator;
	translator.append(first.path());

	EXPECT_THROW(
		translator.append(second.path()),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("first"),
		"First");
	EXPECT_EQ(
		translator.translate("shared"),
		"Shared");
	EXPECT_EQ(
		translator.translate("second"),
		"second");
}

TEST(TranslatorTest, JsonCatalogSupportsUtf8Text)
{
	const TemporaryTranslationFile file(
		R"({"message":"Connexion échouée — réessayez"})");

	spk::Translator translator;
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("message"),
		"Connexion échouée — réessayez");
}

TEST(TranslatorTest, ClearFallsBackToKeys)
{
	spk::Translator translator;
	translator.append("first", "First");
	translator.append("second", "Second");

	translator.clear();

	EXPECT_EQ(
		translator.translate("first"),
		"first");
	EXPECT_EQ(
		translator.translate("second"),
		"second");
}

TEST(TranslatorTest, TranslatorCanBeReusedAfterClear)
{
	spk::Translator translator;
	translator.append("message", "Before");
	translator.clear();
	translator.append("message", "After");

	EXPECT_EQ(
		translator.translate("message"),
		"After");
}

TEST(TranslatorTest, RejectsEmptyDirectKeyWithoutChangingExistingTranslations)
{
	spk::Translator translator;
	translator.append("existing", "Existing");

	EXPECT_THROW(
		translator.append("", "Value"),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
}

TEST(TranslatorTest, RejectsEmptyJsonKeyWithoutPartiallyAppending)
{
	const TemporaryTranslationFile file(
		R"({"valid":"Value","":"Invalid"})");

	spk::Translator translator;
	translator.append("existing", "Existing");

	EXPECT_THROW(
		translator.append(file.path()),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
	EXPECT_EQ(
		translator.translate("valid"),
		"valid");
}

TEST(TranslatorTest, RejectsNonObjectJsonRoot)
{
	const std::vector<std::string> invalidCatalogs = {
		R"([])",
		R"("translation")",
		R"(42)",
		R"(true)",
		R"(null)"};

	for (const std::string &content : invalidCatalogs)
	{
		const TemporaryTranslationFile file(content);
		spk::Translator translator;

		EXPECT_THROW(
			translator.append(file.path()),
			spk::Exception);
	}
}

TEST(TranslatorTest, RejectsNonStringJsonValuesWithoutPartiallyAppending)
{
	const std::vector<std::string> invalidCatalogs = {
		R"({"valid":"Value","invalid":42})",
		R"({"valid":"Value","invalid":true})",
		R"({"valid":"Value","invalid":null})",
		R"({"valid":"Value","invalid":[]})",
		R"({"valid":"Value","invalid":{}})"};

	for (const std::string &content : invalidCatalogs)
	{
		const TemporaryTranslationFile file(content);
		spk::Translator translator;
		translator.append("existing", "Existing");

		EXPECT_THROW(
			translator.append(file.path()),
			spk::Exception);
		EXPECT_EQ(
			translator.translate("existing"),
			"Existing");
		EXPECT_EQ(
			translator.translate("valid"),
			"valid");
	}
}

TEST(TranslatorTest, RejectsMalformedJsonWithoutChangingExistingTranslations)
{
	const TemporaryTranslationFile file(
		R"({"message":"unterminated")");

	spk::Translator translator;
	translator.append("existing", "Existing");

	EXPECT_THROW(
		translator.append(file.path()),
		std::exception);
	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
}

TEST(TranslatorTest, RejectsMissingJsonFileWithoutChangingExistingTranslations)
{
	const std::filesystem::path missing =
		std::filesystem::temp_directory_path() /
		("sparkle-translator-missing-" +
		 spk::UUID::generate().toString() +
		 ".json");

	spk::Translator translator;
	translator.append("existing", "Existing");

	EXPECT_THROW(
		translator.append(missing),
		std::exception);
	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
}

TEST(TranslatorTest, UnknownKeyReturnsKeyAndLogsWarning)
{
	spk::Translator translator;
	std::vector<std::pair<spk::Logger::Level, std::string>> entries;
	auto contract = spk::logger.subscribeToEntry(
		[&](const spk::Logger::Level &level, const std::string &message) {
			entries.emplace_back(level, message);
		});

	EXPECT_EQ(
		translator.translate("unknown"),
		"unknown");

	ASSERT_FALSE(entries.empty());
	EXPECT_EQ(entries.back().first, spk::Logger::Level::Warning);
	EXPECT_EQ(entries.back().second, "Missing translation key: unknown");
}

TEST(TranslatorTest, MissingTranslationLogUsesTranslationCallSite)
{
	spk::Translator translator;
	const std::filesystem::path path =
		std::filesystem::temp_directory_path() /
		("sparkle-translator-location-" +
		 spk::UUID::generate().toString() +
		 ".log");
	std::filesystem::remove(path);

	int translationLine = 0;
	{
		auto output =
			spk::logger.addOutput(
				path,
				spk::Logger::Level::Trace);

		translationLine = __LINE__ + 1;
		const std::string translated = translator.translate("missing.location");
		EXPECT_EQ(translated, "missing.location");
	}

	std::ifstream stream(path);
	std::string line;
	ASSERT_TRUE(static_cast<bool>(std::getline(stream, line)));

	EXPECT_NE(
		line.find(
			"translator_test.cpp:" +
			std::to_string(translationLine)),
		std::string::npos);
	EXPECT_NE(
		line.find(
			"Missing translation key: missing.location"),
		std::string::npos);
	EXPECT_EQ(
		line.find("translator.cpp:"),
		std::string::npos);

	std::filesystem::remove(path);
}

TEST(TranslatorTest, RejectsMalformedFormatString)
{
	spk::Translator translator;
	translator.append("invalid", "Value {");

	EXPECT_THROW(
		(void)translator.translate("invalid", 1),
		spk::Exception);
}

TEST(TranslatorTest, RejectsMissingFormatArguments)
{
	spk::Translator translator;
	translator.append(
		"invalid",
		"{} {}");

	EXPECT_THROW(
		(void)translator.translate("invalid", 1),
		spk::Exception);
}

TEST(TranslatorTest, ConcurrentTranslationAndDistinctAppendRemainValid)
{
	spk::Translator translator;
	translator.append("message", "Value {}");

	std::atomic_bool valid = true;

	std::jthread writer([&] {
		for (std::size_t index = 0; index < 2'000; ++index)
		{
			translator.append(
				"dynamic." + std::to_string(index),
				"Dynamic");
		}
	});

	std::vector<std::jthread> readers;
	readers.reserve(8);
	for (std::size_t readerIndex = 0; readerIndex < 8; ++readerIndex)
	{
		readers.emplace_back([&] {
			for (std::size_t index = 0; index < 2'000; ++index)
			{
				try
				{
					if (
						translator.translate("message", index) !=
						"Value " + std::to_string(index))
					{
						valid = false;
						return;
					}
				}
				catch (...)
				{
					valid = false;
					return;
				}
			}
		});
	}

	writer.join();
	for (std::jthread &reader : readers)
	{
		reader.join();
	}

	EXPECT_TRUE(valid.load());
}


TEST(TranslatorTest, MissingTranslationLogDoesNotCorruptOuterLogComposition)
{
	spk::Translator translator;
	std::vector<std::pair<spk::Logger::Level, std::string>> entries;
	auto contract = spk::logger.subscribeToEntry(
		[&](const spk::Logger::Level &level, const std::string &message) {
			entries.emplace_back(level, message);
		});

	SPK_LOG(Error)
		<< "before "
		<< translator.translate("missing.translation")
		<< " after"
		<< std::endl;

	ASSERT_GE(entries.size(), 2u);
	EXPECT_EQ(
		entries[entries.size() - 2].first,
		spk::Logger::Level::Warning);
	EXPECT_EQ(
		entries[entries.size() - 2].second,
		"Missing translation key: missing.translation");
	EXPECT_EQ(
		entries.back().first,
		spk::Logger::Level::Error);
	EXPECT_EQ(
		entries.back().second,
		"before missing.translation after");
}
