#include "voxel/rendering_system.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "rendering/command/draw_voxel_command.hpp"
#include "voxel/renderer.hpp"

namespace spk::Voxel
{
	namespace
	{
		struct BucketKey
		{
			const std::vector<Definition::Slot> *slots;
			const spk::Palette::Materials *materials;

			[[nodiscard]] bool operator==(const BucketKey &) const = default;
		};

		struct BucketHash
		{
			[[nodiscard]] std::size_t operator()(const BucketKey &key) const noexcept
			{
				const auto geometry = std::hash<const void *>{}(key.slots);
				const auto palette = std::hash<const void *>{}(key.materials);
				return geometry ^ (palette + 0x9e3779b9 + (geometry << 6) + (geometry >> 2));
			}
		};

		struct Bucket
		{
			Geometry geometry;
			spk::Palette palette;
			std::vector<std::uint32_t> materialRemap;
			std::vector<spk::Matrix4x4> models;
		};

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

		void appendBucket(
			std::vector<Bucket> &buckets,
			std::unordered_map<BucketKey, std::size_t, BucketHash> &indices,
			const Renderer &renderer,
			const spk::Matrix4x4 &model)
		{
			const BucketKey key{&renderer.geometry()->materialSlots(), &renderer.palette()->materials()};
			const auto [position, inserted] = indices.try_emplace(key, buckets.size());
			if (inserted)
			{
				buckets.push_back({*renderer.geometry(), *renderer.palette(), renderer.materialRemap(), {model}});
			}
			else
			{
				buckets[position->second].models.push_back(model);
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
		std::vector<Bucket> buckets;
		std::unordered_map<BucketKey, std::size_t, BucketHash> indices;
		for (const Renderer *renderer : spk::Registry<Renderer, spk::EngineHeadless *>::instance().elements(engine()))
		{
			if (!renderer->canContribute() || !context.isLayerCompatible(renderer->layers()) ||
				renderer->owner() == nullptr || renderer->geometry() == nullptr || renderer->palette() == nullptr)
			{
				continue;
			}
			if (renderer->geometry()->opaqueMesh().empty())
			{
				continue;
			}
			const auto &transform = renderer->owner()->transform();
			validateScale(transform.scale(spk::ReferenceFrame::World));
			appendBucket(buckets, indices, *renderer, transform.modelMatrix());
		}
		for (auto &bucket : buckets)
		{
			builder.renderPass(spk::EngineView::OpaquePassKey).emplace<spk::DrawVoxelCommand>(
				std::move(bucket.geometry),
				bucket.palette,
				std::move(bucket.materialRemap),
				std::move(bucket.models),
				context.viewProjectionMatrix);
		}
	}
}
