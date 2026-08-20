#pragma once

namespace spk
{
	struct RenderContext;

	class RenderCommand
	{
		// Abstract render command interface.

	public:
		virtual ~RenderCommand() = default;
		virtual void execute(RenderContext &renderContext) const = 0;
	};
}