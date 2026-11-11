#pragma once

#include "rendering/engine/rendering_system.hpp"

namespace spk::Voxel
{
	class RenderingSystem final : public spk::RenderingSystem
	{
	public:
		RenderingSystem();

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context) override;
	};
}
