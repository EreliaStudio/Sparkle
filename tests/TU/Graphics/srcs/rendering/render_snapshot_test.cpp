#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "core/context/render_context.hpp"
#include "exception.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"

namespace
{
	class SnapshotRecordingCommand final : public spk::RenderCommand
	{
	private:
		std::vector<int> *_log;
		int _value;

	public:
		SnapshotRecordingCommand(std::vector<int> &log, int value) : _log(&log), _value(value) {}
		void execute(spk::RenderContext &) const override { _log->push_back(_value); }
	};

	class SnapshotThrowingCommand final : public spk::RenderCommand
	{
	public:
		void execute(spk::RenderContext &) const override { throw std::runtime_error("command failure"); }
	};
}

TEST(RenderSnapshotTest, OrdersPassesByKeyAndPreservesInsertionWithinEqualOrders)
{
	std::vector<int> log;
	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"late", 20}).emplace<SnapshotRecordingCommand>(log, 4);
	builder.renderPass({"equal-a", 10}).emplace<SnapshotRecordingCommand>(log, 2);
	builder.renderPass({"early", -5}).emplace<SnapshotRecordingCommand>(log, 1);
	builder.renderPass({"equal-b", 10}).emplace<SnapshotRecordingCommand>(log, 3);
	builder.renderPass({"late", 20}).emplace<SnapshotRecordingCommand>(log, 5);
	const auto snapshot = builder.build();
	spk::RenderContext context{.targetSurface = nullptr};
	snapshot.execute(context);
	EXPECT_EQ(log, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(RenderSnapshotTest, EmptyAndReusedBuilderProduceIndependentSnapshots)
{
	spk::RenderSnapshot::Builder builder;
	const auto empty = builder.build();
	std::vector<int> log;
	builder.renderPass({"next", 0}).emplace<SnapshotRecordingCommand>(log, 1);
	const auto populated = builder.build();
	spk::RenderContext context{.targetSurface = nullptr};
	EXPECT_NO_THROW(empty.execute(context));
	populated.execute(context);
	EXPECT_EQ(log, (std::vector<int>{1}));
	EXPECT_NO_THROW(builder.build().execute(context));
}

TEST(RenderSnapshotTest, SameNameWithDifferentOrderIsRejectedWithoutLosingOriginalPass)
{
	spk::RenderSnapshot::Builder builder;
	std::vector<int> log;
	builder.renderPass({"same", 1}).emplace<SnapshotRecordingCommand>(log, 7);
	EXPECT_THROW(builder.renderPass({"same", 2}), spk::RenderSnapshot::Builder::InvalidRenderPassKeyError);
	spk::RenderContext context{.targetSurface = nullptr};
	builder.build().execute(context);
	EXPECT_EQ(log, (std::vector<int>{7}));
}

TEST(RenderSnapshotTest, FailureIdentifiesPassAndNestedCommandIndex)
{
	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"broken", 0}).emplace<SnapshotThrowingCommand>();
	spk::RenderContext context{.targetSurface = nullptr};
	try
	{
		builder.build().execute(context);
		FAIL() << "Expected spk::Exception";
	}
	catch (const spk::Exception &exception)
	{
		const std::string message = exception.what();
		EXPECT_NE(message.find("render pass [broken]"), std::string::npos);
		EXPECT_NE(message.find("render command [0]"), std::string::npos);
		EXPECT_NE(exception.cause(), nullptr);
	}
}
