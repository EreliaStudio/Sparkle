#include "voxel/model_view.hpp"

#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace spk
{
	namespace
	{
		using Geometry = spk::Voxel::Geometry;
		using SlotKey = std::pair<std::string, float>;
		using Slots = std::set<SlotKey>;
		using Indices = std::map<SlotKey, std::uint32_t>;
		using PartView = spk::Model::Skeleton::Part::View;

		void collectSlots(const PartView &view, Slots &slots)
		{
			if (const auto *geometry = view.geometry())
			{
				for (const auto &slot : geometry->materialSlots())
				{
					slots.emplace(slot.name, slot.opacity);
				}
			}
			for (const auto &[name, joint] : view.part().joints())
			{
				if (joint.child())
				{
					collectSlots(view.child(name), slots);
				}
			}
		}

		[[nodiscard]] Geometry::SurfaceMesh remapSurface(
			const Geometry::SurfaceMesh &source, const Geometry &geometry, const Indices &indices)
		{
			if (source.vertexCount() == 0)
			{
				return source;
			}
			const auto vertices = source.layout().vertexBuffer().cast<Geometry::Vertex>();
			const auto triangles = source.layout().indexBuffer().cast<std::uint32_t>();
			Geometry::SurfaceMesh::Builder builder;
			builder.reserve(vertices.size(), triangles.size());
			for (auto vertex : vertices)
			{
				const auto &slot = geometry.materialSlot(vertex.materialIndex);
				vertex.materialIndex = indices.at({slot.name, slot.opacity});
				(void)builder.addVertex(vertex);
			}
			for (const auto index : triangles)
			{
				builder.addIndex(index);
			}
			return std::move(builder).build();
		}

		[[nodiscard]] Geometry remapGeometry(
			const Geometry &source, const std::vector<spk::Voxel::Definition::Slot> &slots, const Indices &indices)
		{
			return Geometry(
				remapSurface(source.opaqueMesh(), source, indices),
				remapSurface(source.transparentMesh(), source, indices),
				source.depthMesh(),
				slots);
		}

		[[nodiscard]] std::vector<spk::Voxel::Definition::Slot> sharedSlots(const Slots &collected, Indices &indices)
		{
			if (collected.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Model View: too many material slots");
			}
			std::vector<spk::Voxel::Definition::Slot> slots;
			slots.reserve(collected.size());
			for (const auto &[name, opacity] : collected)
			{
				indices.emplace(name, static_cast<std::uint32_t>(slots.size()));
				slots.push_back({name, opacity});
			}
			return slots;
		}
	}

	void Model::View::harmonize()
	{
		Slots collected;
		collectSlots(root(), collected);
		Indices indices;
		const auto slots = sharedSlots(collected, indices);
		std::vector<std::pair<Skeleton::Part::View *, Geometry>> prepared;
		std::function<void(Skeleton::Part::View &)> rewrite = [&](Skeleton::Part::View &view) {
			if (const auto *geometry = view.geometry())
			{
				prepared.emplace_back(&view, remapGeometry(*geometry, slots, indices));
			}
			for (const auto &[name, joint] : view.part().joints())
			{
				if (joint.child())
				{
					rewrite(view.child(name));
				}
			}
		};
		rewrite(root());
		for (auto &[view, geometry] : prepared)
		{
			view->_geometry = std::move(geometry);
		}
	}
}
