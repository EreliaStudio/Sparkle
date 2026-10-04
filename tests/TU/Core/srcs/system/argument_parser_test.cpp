#include <system/argument_parser.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{
	void parse(
		spk::ArgumentParser &parser,
		std::vector<std::string> arguments)
	{
		std::vector<char *> argv;
		argv.reserve(arguments.size());
		for (std::string &argument : arguments)
		{
			argv.push_back(argument.data());
		}

		parser.parse(
			static_cast<int>(argv.size()),
			argv.data());
	}
}

TEST(ArgumentParser, ParsesLongShortAndInlineValues)
{
	spk::ArgumentParser parser;
	parser.addOption({"config", 'c', "Configuration file", 1});
	parser.addOption({"verbose", 'v', "Verbose output"});

	parse(
		parser,
		{"program", "--config=router.json", "-v"});

	ASSERT_TRUE(parser.has("config"));
	ASSERT_TRUE(parser.has("verbose"));
	ASSERT_EQ(parser.get("config").values.size(), 1u);
	EXPECT_EQ(parser.get("config").values.front(), "router.json");
	EXPECT_TRUE(parser.get('v').present);
}

TEST(ArgumentParser, AcceptsSeparatedValuesAndMultipleArity)
{
	spk::ArgumentParser parser;
	parser.addOption({"range", 'r', "Range", 2});

	parse(
		parser,
		{"program", "--range", "12", "42"});

	ASSERT_EQ(parser.get("range").values.size(), 2u);
	EXPECT_EQ(parser.get("range").values[0], "12");
	EXPECT_EQ(parser.get("range").values[1], "42");
}

TEST(ArgumentParser, AppliesDefaultWithoutMarkingOptionPresent)
{
	spk::ArgumentParser parser;
	parser.addOption(
		{"config", 'c', "Configuration file", 1, "server.json"});

	parse(parser, {"program"});

	EXPECT_FALSE(parser.has("config"));
	ASSERT_EQ(parser.get("config").values.size(), 1u);
	EXPECT_EQ(parser.get("config").values.front(), "server.json");
}

TEST(ArgumentParser, PreservesPositionalParametersAndEndOfOptionsMarker)
{
	spk::ArgumentParser parser;
	parser.addOption({"config", 'c', "Configuration file", 1});

	parse(
		parser,
		{"program", "before", "--", "--config", "after"});

	ASSERT_EQ(parser.parameters().size(), 3u);
	EXPECT_EQ(parser.parameters()[0], "before");
	EXPECT_EQ(parser.parameters()[1], "--config");
	EXPECT_EQ(parser.parameters()[2], "after");
	EXPECT_FALSE(parser.has("config"));
}

TEST(ArgumentParser, ParseResetsPreviousState)
{
	spk::ArgumentParser parser;
	parser.addOption({"config", 'c', "Configuration file", 1});

	parse(parser, {"program", "--config=first.json", "old"});
	parse(parser, {"program", "--config", "second.json"});

	ASSERT_EQ(parser.get("config").values.size(), 1u);
	EXPECT_EQ(parser.get("config").values.front(), "second.json");
	EXPECT_TRUE(parser.parameters().empty());
}

TEST(ArgumentParser, RejectsInvalidRegistrationAndArguments)
{
	spk::ArgumentParser parser;
	parser.addOption({"config", 'c', "Configuration file", 1});
	parser.addOption({"verbose", 'v', "Verbose output"});

	EXPECT_THROW(
		parser.addOption({"config", 'x', "Duplicate long"}),
		spk::Exception);
	EXPECT_THROW(
		parser.addOption({"other", 'c', "Duplicate short"}),
		spk::Exception);
	EXPECT_THROW(
		parser.addOption({"", '\0', "No name"}),
		spk::Exception);

	EXPECT_THROW(
		parse(parser, {"program", "--unknown"}),
		spk::Exception);
	EXPECT_THROW(
		parse(parser, {"program", "-vc"}),
		spk::Exception);
	EXPECT_THROW(
		parse(parser, {"program", "--config"}),
		spk::Exception);
	EXPECT_THROW(
		parse(parser, {"program", "--verbose=yes"}),
		spk::Exception);
}

TEST(ArgumentParser, PrintsSynopsisOptionAndDefault)
{
	spk::ArgumentParser parser;
	parser.setSynopsis("program --config <path>");
	parser.addOption(
		{"config", 'c', "Configuration file", 1, "server.json"});

	std::ostringstream output;
	parser.printHelp(output);

	const std::string help = output.str();
	EXPECT_NE(
		help.find("Usage: program --config <path>"),
		std::string::npos);
	EXPECT_NE(help.find("--config, -c <1 value>"), std::string::npos);
	EXPECT_NE(help.find("(default: server.json)"), std::string::npos);
	EXPECT_NE(help.find("Configuration file"), std::string::npos);
}
