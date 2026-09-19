#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_pass.hpp"

namespace spk
{
	class RenderSnapshot
	{
	public:
		class Builder
		{
		private:
			struct PendingPass
			{
				RenderPass::Key key;
				std::unique_ptr<RenderPass> pass;
			};

		public:
			class InvalidRenderPassKeyError : public std::logic_error
			{
			public:
				using std::logic_error::logic_error;
			};

			RenderPass &renderPass(const RenderPass::Key &key);
			RenderSnapshot build();

		private:
			[[nodiscard]] PendingPass *findPass(const std::string &name);

			std::vector<PendingPass> _passes;
		};

		RenderSnapshot() = default;
		void execute(RenderContext &renderContext) const;

	private:
		struct Pass
		{
			RenderPass::Name name;
			std::unique_ptr<const RenderPass> commands;
		};

		explicit RenderSnapshot(
			std::vector<Pass> passes);

		std::vector<Pass> _renderPasses;
	};
}
