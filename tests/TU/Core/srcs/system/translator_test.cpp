#include <system/translator.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>
#include <type/uuid.hpp>

#include <filesystem>
#include <fstream>
#include <string>

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

TEST(TranslatorTest, AppendsAndFormatsTranslation)
{
	spk::Translator translator;
	translator.append(
		"connection.failed",
		"Unable to connect after {} attempts");

	EXPECT_EQ(
		translator.translate("connection.failed", 5),
		"Unable to connect after 5 attempts");
}

TEST(TranslatorTest, AppendsJsonCatalog)
{
	const TemporaryTranslationFile file(
		R"({"first":"First {}","ordered":"{1} then {0}"})");

	spk::Translator translator;
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("first", 1),
		"First 1");
	EXPECT_EQ(
		translator.translate(
			"ordered",
			"first",
			"second"),
		"second then first");
}

TEST(TranslatorTest, LaterAppendOverridesExistingTranslation)
{
	const TemporaryTranslationFile file(
		R"({"message":"From file"})");

	spk::Translator translator;
	translator.append("message", "Initial");
	translator.append(file.path());

	EXPECT_EQ(
		translator.translate("message"),
		"From file");

	translator.append("message", "Final");

	EXPECT_EQ(
		translator.translate("message"),
		"Final");
}

TEST(TranslatorTest, ClearRemovesTranslations)
{
	spk::Translator translator;
	translator.append("message", "Value");

	translator.clear();

	EXPECT_THROW(
		(void)translator.translate("message"),
		spk::Exception);
}

TEST(TranslatorTest, InvalidCatalogDoesNotPartiallyAppend)
{
	const TemporaryTranslationFile file(
		R"({"valid":"Value","invalid":42})");

	spk::Translator translator;
	translator.append("existing", "Existing");

	EXPECT_THROW(
		translator.append(file.path()),
		spk::Exception);
	EXPECT_EQ(
		translator.translate("existing"),
		"Existing");
	EXPECT_THROW(
		(void)translator.translate("valid"),
		spk::Exception);
}

TEST(TranslatorTest, RejectsEmptyKey)
{
	spk::Translator translator;

	EXPECT_THROW(
		translator.append("", "Value"),
		spk::Exception);
}

TEST(TranslatorTest, RejectsUnknownKey)
{
	spk::Translator translator;

	EXPECT_THROW(
		(void)translator.translate("unknown"),
		spk::Exception);
}

TEST(TranslatorTest, RejectsInvalidFormat)
{
	spk::Translator translator;
	translator.append("invalid", "Value {");

	EXPECT_THROW(
		(void)translator.translate("invalid", 1),
		spk::Exception);
}
