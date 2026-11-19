#pragma once

#include <memory>
#include <utility>

#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	class ExecuteSnapshotRenderCommand final : public RenderCommand
	{
	private:
		std::shared_ptr<const RenderSnapshot> _snapshot;

	public:
		explicit ExecuteSnapshotRenderCommand(std::shared_ptr<const RenderSnapshot> snapshot) :
			_snapshot(std::move(snapshot))
		{
		}

		void execute(RenderContext &context) const override
		{
			_snapshot->execute(context);
		}
	};
}
