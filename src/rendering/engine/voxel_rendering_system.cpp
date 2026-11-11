#include "voxel/rendering_system.hpp"

#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "voxel/model_renderer.hpp"
#include "voxel/render_batch.hpp"
#include "voxel/renderer.hpp"

namespace spk::Voxel
{
	RenderingSystem::RenderingSystem() :
		spk::RenderingSystem("Voxel rendering")
	{
	}

	void RenderingSystem::buildViewRenderSnapshot(
		spk::RenderSnapshot::Builder &builder,
		const spk::EngineView::CaptureContext &context)
	{
		RenderBatch batch;
		for (const Renderer *renderer : spk::Registry<Renderer, spk::EngineHeadless *>::instance().elements(engine()))
		{
			if (!renderer->canContribute() || !context.isLayerCompatible(renderer->layers()) ||
				renderer->owner() == nullptr || renderer->geometry() == nullptr || renderer->palette() == nullptr)
			{
				continue;
			}
			const auto &transform = renderer->owner()->transform();
			RenderBatch::validateScale(transform.scale(spk::ReferenceFrame::World));
			batch.append(*renderer->geometry(), *renderer->palette(), renderer->materialRemap(), transform.modelMatrix());
		}
		for (ModelRenderer *renderer : spk::Registry<ModelRenderer, spk::EngineHeadless *>::instance().elements(engine()))
		{
			if (!renderer->canContribute() || !context.isLayerCompatible(renderer->layers()) ||
				renderer->owner() == nullptr || renderer->model() == nullptr)
			{
				continue;
			}
			const auto &transform = renderer->owner()->transform();
			RenderBatch::validateScale(transform.scale(spk::ReferenceFrame::World));
			renderer->collect(builder, transform.modelMatrix(), batch);
		}
		batch.publish(builder, context.viewProjectionMatrix);
	}
}
