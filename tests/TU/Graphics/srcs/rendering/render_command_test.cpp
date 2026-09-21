#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/context/render_context.hpp"
#include "rendering/render_command.hpp"

namespace
{
	class PolymorphicRecordingCommand final : public spk::RenderCommand
	{
	private:
		std::vector<int> *_log;
		int *_destructions;

	public:
		PolymorphicRecordingCommand(std::vector<int> &log, int &destructions) : _log(&log), _destructions(&destructions) {}
		~PolymorphicRecordingCommand() override { ++*_destructions; }
		void execute(spk::RenderContext &) const override { _log->push_back(9); }
	};
}

TEST(RenderCommandTest, PolymorphicConstOwnershipExecutesAndDestroysExactlyOnce)
{
	std::vector<int> log;
	int destructions = 0;
	spk::RenderContext context{.targetSurface = nullptr};
	{
		std::unique_ptr<const spk::RenderCommand> command = std::make_unique<PolymorphicRecordingCommand>(log, destructions);
		command->execute(context);
	}
	EXPECT_EQ(log, (std::vector<int>{9}));
	EXPECT_EQ(destructions, 1);
}
