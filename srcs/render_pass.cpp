#include "render_pass.hpp"

#include "render_command.hpp"

#include <utility>

namespace spk
{
	RenderPass::RenderPass() = default;
	RenderPass::RenderPass(RenderPass &&) noexcept = default;
	RenderPass::~RenderPass() = default;
	RenderPass &RenderPass::operator=(RenderPass &&) noexcept = default;

	void RenderPass::append(std::unique_ptr<const RenderCommand> renderCommand)
	{
		_commands.push_back(std::move(renderCommand));
	}

	void RenderPass::execute(RenderContext &renderContext) const
	{
		for (const auto &command : _commands)
		{
			command->execute(renderContext);
		}
	}
}
