#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "exception.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_pass.hpp"

namespace
{
	class PassRecordingCommand final : public spk::RenderCommand
	{
	private:
		std::vector<int> *_log;
		int _value;
		int *_destructions;

	public:
		PassRecordingCommand(std::vector<int> &log, int value, int *destructions = nullptr) :
			_log(&log), _value(value), _destructions(destructions) {}
		~PassRecordingCommand() override
		{
			if (_destructions != nullptr)
				++*_destructions;
		}
		void execute(spk::RenderContext &) const override { _log->push_back(_value); }
	};

	class PassThrowingCommand final : public spk::RenderCommand
	{
	public:
		void execute(spk::RenderContext &) const override { throw std::runtime_error("command failure"); }
	};
}

TEST(RenderPassTest, EmptyPassAndRecordingCommandsExecuteInInsertionOrder)
{
	spk::RenderContext context{.targetSurface = nullptr};
	spk::RenderPass empty;
	EXPECT_NO_THROW(empty.execute(context));
	std::vector<int> log;
	spk::RenderPass pass;
	pass.emplace<PassRecordingCommand>(log, 1);
	pass.append(std::make_unique<PassRecordingCommand>(log, 2));
	pass.emplace<PassRecordingCommand>(log, 3);
	pass.execute(context);
	EXPECT_EQ(log, (std::vector<int>{1, 2, 3}));
}

TEST(RenderPassTest, MoveConstructionAndAssignmentTransferCommandsAndLifetime)
{
	spk::RenderContext context{.targetSurface = nullptr};
	std::vector<int> log;
	int destructions = 0;
	{
		spk::RenderPass source;
		source.emplace<PassRecordingCommand>(log, 4, &destructions);
		spk::RenderPass moved(std::move(source));
		spk::RenderPass assigned;
		assigned = std::move(moved);
		assigned.execute(context);
		EXPECT_EQ(destructions, 0);
	}
	EXPECT_EQ(log, (std::vector<int>{4}));
	EXPECT_EQ(destructions, 1);
}

TEST(RenderPassTest, CommandFailureIsWrappedWithIndexAndRetainsCause)
{
	spk::RenderContext context{.targetSurface = nullptr};
	std::vector<int> log;
	spk::RenderPass pass;
	pass.emplace<PassRecordingCommand>(log, 1);
	pass.emplace<PassThrowingCommand>();
	pass.emplace<PassRecordingCommand>(log, 3);
	try
	{
		pass.execute(context);
		FAIL() << "Expected spk::Exception";
	}
	catch (const spk::Exception &exception)
	{
		EXPECT_NE(std::string(exception.what()).find("render command [1]"), std::string::npos);
		EXPECT_NE(exception.cause(), nullptr);
		EXPECT_THROW(std::rethrow_exception(exception.cause()), std::runtime_error);
	}
	EXPECT_EQ(log, (std::vector<int>{1}));
}
