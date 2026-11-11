#include "voxel/rendering_system.hpp"

#include <algorithm>
#include <cmath>

#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "rendering/command/draw_voxel_command.hpp"
#include "voxel/renderer.hpp"

namespace spk::Voxel
{
	namespace
	{
		void validateScale(const spk::Vector3 &scale)
		{
			const auto valid = [](float a, float b) {
				return std::abs(a - b) <= 1.0e-5f * std::max(1.0f, std::abs(a));
			};
			if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z) ||
				scale.x <= 0.0f || !valid(scale.x, scale.y) || !valid(scale.x, scale.z))
			{
				throw spk::Exception("Voxel RenderingSystem: only positive uniform scales are supported");
			}
		}
	}

	RenderingSystem::RenderingSystem() :
		spk::RenderingSystem("Voxel rendering")
	{
	}

	void RenderingSystem::buildViewRenderSnapshot(
		spk::RenderSnapshot::Builder &builder,
		const spk::EngineView::CaptureContext &context)
	{
		for (const Renderer *renderer : spk::Registry<Renderer, spk::EngineHeadless *>::instance().elements(engine()))
		{
			if (!renderer->canContribute() || !context.isLayerCompatible(renderer->layers()) ||
				renderer->owner() == nullptr || renderer->geometry() == nullptr || renderer->palette() == nullptr)
			{
				continue;
			}
			const Geometry &geometry = *renderer->geometry();
			if (geometry.opaqueMesh().empty())
			{
				continue;
			}
			const auto &transform = renderer->owner()->transform();
			validateScale(transform.scale(spk::ReferenceFrame::World));
			builder.renderPass(spk::EngineView::OpaquePassKey).emplace<spk::DrawVoxelCommand>(
				geometry, *renderer->palette(), renderer->materialRemap(), transform.modelMatrix(),
				context.viewProjectionMatrix);
		}
	}
}
