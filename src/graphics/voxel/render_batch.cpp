#include "voxel/render_batch.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>

#include "exception.hpp"
#include "rendering/command/draw_voxel_command.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk::Voxel
{
	std::size_t RenderBatch::KeyHash::operator()(const Key &key) const noexcept
	{
		const auto geometry = std::hash<const void *>{}(key.slots);
		const auto palette = std::hash<const void *>{}(key.materials);
		return geometry ^ (palette + 0x9e3779b9 + (geometry << 6) + (geometry >> 2));
	}

	void RenderBatch::validateScale(const spk::Vector3 &scale)
	{
		const auto uniform = [](float a, float b) {
			return std::abs(a - b) <= 1.0e-5f * std::max(1.0f, std::abs(a));
		};
		if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z) ||
			scale.x <= 0.0f || !uniform(scale.x, scale.y) || !uniform(scale.x, scale.z))
		{
			throw spk::Exception("Voxel renderer: only positive uniform scales are supported");
		}
	}

	void RenderBatch::append(
		Geometry geometry,
		spk::Palette palette,
		std::vector<std::uint32_t> materialRemap,
		const spk::Matrix4x4 &model)
	{
		if (geometry.opaqueMesh().empty())
		{
			return;
		}
		const Key key{&geometry.materialSlots(), &palette.materials()};
		const auto [position, inserted] = _indices.try_emplace(key, _buckets.size());
		if (inserted)
		{
			_buckets.push_back({std::move(geometry), std::move(palette), std::move(materialRemap), {model}});
		}
		else
		{
			_buckets[position->second].models.push_back(model);
		}
	}

	void RenderBatch::publish(spk::RenderSnapshot::Builder &builder, const spk::Matrix4x4 &viewProjection)
	{
		for (auto &bucket : _buckets)
		{
			builder.renderPass(spk::EngineView::OpaquePassKey)
				.emplace<spk::DrawVoxelCommand>(
					std::move(bucket.geometry),
					bucket.palette,
					std::move(bucket.materialRemap),
					std::move(bucket.models),
					viewProjection);
		}
		_buckets.clear();
		_indices.clear();
	}

	std::size_t RenderBatch::bucketCount() const noexcept
	{
		return _buckets.size();
	}
}
