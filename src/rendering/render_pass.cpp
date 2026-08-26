#include "rendering/render_pass.hpp"

#include "exception.hpp"
#include "rendering/render_command.hpp"

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
		for (std::size_t index = 0; index < _commands.size(); ++index)
		{
			try
			{
				_commands[index]->execute(renderContext);
			} catch (spk::Exception &exception)
			{
				exception.addContext("Exception while executing render command [" + std::to_string(index) + "]");
				throw;
			} catch (...)
			{
				throw spk::Exception(
					"Exception while executing render command [" + std::to_string(index) + "]",
					std::current_exception());
			}
		}
	}
}
