#include <system/command_parser.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(CommandParserTest, AcceptsNamedInlineAndSeparatedValues)
{
	spk::CommandParser parser;
	std::vector<std::string> received;
	parser.addCommand({.name = "sample", .description = "Sample command", .parameters = {{.name = "value", .description = "Values", .arity = 3}}, .callback = [&received](const spk::CommandParser::Invocation &invocation) {
						   received = invocation.get("value");
					   }});

	EXPECT_EQ(parser.execute("/sample --value A B C").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(received, (std::vector<std::string>{"A", "B", "C"}));
	EXPECT_EQ(parser.execute("/sample --value=A,B,C").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(received, (std::vector<std::string>{"A", "B", "C"}));
}

TEST(CommandParserTest, AcceptsPositionalNamedAndMixedParameters)
{
	spk::CommandParser parser;
	std::string address;
	std::string port;
	parser.addCommand({.name = "connect", .description = "Connect", .parameters = {{.name = "address", .description = "Address"}, {.name = "port", .description = "Port"}}, .callback = [&](const spk::CommandParser::Invocation &invocation) {
						   address = invocation.get("address").front();
						   port = invocation.get("port").front();
					   }});

	EXPECT_EQ(parser.execute("/connect localhost 2550").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(address, "localhost");
	EXPECT_EQ(port, "2550");
	EXPECT_EQ(parser.execute("/connect localhost --port=3000").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(address, "localhost");
	EXPECT_EQ(port, "3000");
	EXPECT_EQ(parser.execute("/connect --port 4000 localhost").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(address, "localhost");
	EXPECT_EQ(port, "4000");
}

TEST(CommandParserTest, DefaultsFillMissingParameters)
{
	spk::CommandParser parser;
	std::vector<std::string> received;
	parser.addCommand({.name = "sample", .description = "Sample", .parameters = {{.name = "value", .description = "Value", .arity = 2, .defaultValues = {"A", "B"}}}, .callback = [&](const spk::CommandParser::Invocation &invocation) {
						   received = invocation.get("value");
					   }});

	EXPECT_EQ(parser.execute("/sample").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(received, (std::vector<std::string>{"A", "B"}));
}

TEST(CommandParserTest, RejectsDuplicateParameterWithDiagnosticContext)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "sample", .description = "Sample", .parameters = {{.name = "value", .description = "Value"}}});

	const auto result = parser.execute("/sample --value A --value B");
	EXPECT_EQ(result.status, spk::CommandParser::Status::DuplicateParameter);
	EXPECT_EQ(result.command, "sample");
	EXPECT_EQ(result.parameter, "value");
}

TEST(CommandParserTest, ReportsNormalInputFailuresWithoutExecutingCallback)
{
	spk::CommandParser parser;
	std::size_t calls = 0;
	parser.addCommand({.name = "sample", .description = "Sample", .parameters = {{.name = "value", .description = "Values", .arity = 3}}, .callback = [&](const spk::CommandParser::Invocation &) {
						   ++calls;
					   }});

	auto result = parser.execute("sample");
	EXPECT_EQ(result.status, spk::CommandParser::Status::InvalidFormat);
	result = parser.execute("/unknown");
	EXPECT_EQ(result.status, spk::CommandParser::Status::UnknownCommand);
	EXPECT_EQ(result.command, "unknown");
	result = parser.execute("/sample --unknown A");
	EXPECT_EQ(result.status, spk::CommandParser::Status::UnknownParameter);
	EXPECT_EQ(result.parameter, "unknown");
	result = parser.execute("/sample --value A");
	EXPECT_EQ(result.status, spk::CommandParser::Status::MissingValue);
	EXPECT_EQ(result.parameter, "value");
	EXPECT_EQ(result.expectedValueCount, 3u);
	EXPECT_EQ(result.actualValueCount, 1u);
	result = parser.execute("/sample --value=A,B,C,D");
	EXPECT_EQ(result.status, spk::CommandParser::Status::TooManyValues);
	EXPECT_EQ(result.expectedValueCount, 3u);
	EXPECT_EQ(result.actualValueCount, 4u);
	EXPECT_EQ(calls, 0u);
}

TEST(CommandParserTest, ReportsMissingAndExtraPositionalParameters)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "sample", .description = "Sample", .parameters = {{.name = "required", .description = "Required"}}});

	auto result = parser.execute("/sample");
	EXPECT_EQ(result.status, spk::CommandParser::Status::MissingParameter);
	EXPECT_EQ(result.parameter, "required");
	result = parser.execute("/sample A B");
	EXPECT_EQ(result.status, spk::CommandParser::Status::TooManyParameters);
}

TEST(CommandParserTest, HelpDoesNotExecuteCommand)
{
	spk::CommandParser parser;
	bool executed = false;
	parser.addCommand({.name = "sample", .description = "Sample description", .parameters = {{.name = "value", .description = "Value description", .defaultValues = {"default"}}}, .callback = [&](const spk::CommandParser::Invocation &) {
						   executed = true;
					   }});

	const auto result = parser.execute("/sample --help");
	EXPECT_EQ(result.status, spk::CommandParser::Status::HelpRequested);
	EXPECT_EQ(result.command, "sample");
	EXPECT_FALSE(executed);
	const std::string help = parser.help("sample");
	EXPECT_NE(help.find("Usage: /sample"), std::string::npos);
	EXPECT_NE(help.find("--value"), std::string::npos);
	EXPECT_NE(help.find("default"), std::string::npos);
	EXPECT_NE(parser.help().find("/sample - Sample description"), std::string::npos);
}

TEST(CommandParserTest, RejectsInvalidRegistrations)
{
	spk::CommandParser parser;
	EXPECT_THROW(parser.addCommand({.name = "", .description = ""}), spk::Exception);
	parser.addCommand({.name = "sample", .description = "Sample"});
	EXPECT_THROW(parser.addCommand({.name = "sample", .description = "Duplicate"}), spk::Exception);
	EXPECT_THROW(
		parser.addCommand({.name = "invalid", .description = "Invalid", .parameters = {{.name = "value", .description = "Value", .arity = 2, .defaultValues = {"only-one"}}}}),
		spk::Exception);
}

TEST(CommandParserTest, OmittedOptionalParameterIsAbsentFromInvocation)
{
	spk::CommandParser parser;
	bool executed = false;
	parser.addCommand({.name = "optional", .description = "Optional", .parameters = {{.name = "value", .description = "Value", .optional = true}}, .callback = [&](const spk::CommandParser::Invocation &invocation) {
						   executed = true;
						   EXPECT_FALSE(invocation.parameters.contains("value"));
					   }});

	EXPECT_EQ(parser.execute("/optional").status, spk::CommandParser::Status::Accepted);
	EXPECT_TRUE(executed);
}

TEST(CommandParserTest, PresentOptionalParameterIsAvailable)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "optional", .description = "Optional", .parameters = {{.name = "value", .description = "Value", .optional = true}}, .callback = [](const spk::CommandParser::Invocation &invocation) {
						   EXPECT_EQ(invocation.get("value"), (std::vector<std::string>{"provided"}));
					   }});

	EXPECT_EQ(parser.execute("/optional --value provided").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(parser.execute("/optional --value=provided").status, spk::CommandParser::Status::Accepted);
}

TEST(CommandParserTest, OptionalParameterStillValidatesPresentValue)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "optional", .description = "Optional", .parameters = {{.name = "value", .description = "Value", .arity = 2, .optional = true}}});

	auto result = parser.execute("/optional --value");
	EXPECT_EQ(result.status, spk::CommandParser::Status::MissingValue);
	EXPECT_EQ(result.expectedValueCount, 2u);
	EXPECT_EQ(result.actualValueCount, 0u);

	result = parser.execute("/optional --value one");
	EXPECT_EQ(result.status, spk::CommandParser::Status::MissingValue);
	EXPECT_EQ(result.actualValueCount, 1u);

	result = parser.execute("/optional --value=one,two,three");
	EXPECT_EQ(result.status, spk::CommandParser::Status::TooManyValues);
	EXPECT_EQ(result.actualValueCount, 3u);
}

TEST(CommandParserTest, OptionalParameterWithDefaultStillReceivesDefault)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "optional", .description = "Optional", .parameters = {{.name = "value", .description = "Value", .defaultValues = {"default"}, .optional = true}}, .callback = [](const spk::CommandParser::Invocation &invocation) {
						   EXPECT_EQ(invocation.get("value"), (std::vector<std::string>{"default"}));
					   }});

	EXPECT_EQ(parser.execute("/optional").status, spk::CommandParser::Status::Accepted);
}

TEST(CommandParserTest, InvocationGetRejectsAbsentOptionalParameter)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "optional", .description = "Optional", .parameters = {{.name = "value", .description = "Value", .optional = true}}, .callback = [](const spk::CommandParser::Invocation &invocation) {
						   EXPECT_THROW((void)invocation.get("value"), spk::Exception);
					   }});

	EXPECT_EQ(parser.execute("/optional").status, spk::CommandParser::Status::Accepted);
}

TEST(CommandParserTest, MultipleOptionalParametersMayBeIndependentlyOmitted)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "connect", .description = "Connect", .parameters = {{.name = "address", .description = "Address", .optional = true}, {.name = "port", .description = "Port", .optional = true}}, .callback = [](const spk::CommandParser::Invocation &invocation) {
						   EXPECT_FALSE(invocation.parameters.contains("address"));
						   ASSERT_TRUE(invocation.parameters.contains("port"));
						   EXPECT_EQ(invocation.get("port").front(), "2550");
					   }});

	EXPECT_EQ(parser.execute("/connect --port 2550").status, spk::CommandParser::Status::Accepted);
}

TEST(CommandParserTest, HelpRequestWithAdditionalTokensIsInvalid)
{
	spk::CommandParser parser;
	parser.addCommand({.name = "sample", .description = "Sample"});
	EXPECT_EQ(parser.execute("/sample --help extra").status, spk::CommandParser::Status::InvalidFormat);
}

TEST(CommandParserTest, UnknownCommandHelpIsEmpty)
{
	spk::CommandParser parser;
	EXPECT_TRUE(parser.help("unknown").empty());
}


namespace
{
	class RecordingCommand final : public spk::CommandParser::Command
	{
	private:
		std::vector<std::string> &_received;

	public:
		explicit RecordingCommand(std::vector<std::string> &received) :
			Command(
				"custom",
				"Polymorphic command",
				{{.name = "value", .description = "Value"}}),
			_received(received)
		{
		}

		void execute(const spk::CommandParser::Invocation &invocation) override
		{
			_received = invocation.get("value");
		}
	};
}

TEST(CommandParserTest, AcceptsPolymorphicCommandImplementation)
{
	spk::CommandParser parser;
	std::vector<std::string> received;
	RecordingCommand &command = parser.addCommand<RecordingCommand>(received);

	EXPECT_EQ(command.name(), "custom");
	EXPECT_EQ(parser.execute("/custom payload").status, spk::CommandParser::Status::Accepted);
	EXPECT_EQ(received, (std::vector<std::string>{"payload"}));
	EXPECT_NE(parser.help().find("/custom - Polymorphic command"), std::string::npos);
	EXPECT_NE(parser.help("custom").find("--value"), std::string::npos);
}

TEST(CommandParserTest, RejectsDuplicatePolymorphicCommandName)
{
	spk::CommandParser parser;
	std::vector<std::string> first;
	std::vector<std::string> second;
	parser.addCommand<RecordingCommand>(first);

	EXPECT_THROW(
		parser.addCommand<RecordingCommand>(second),
		spk::Exception);
}
