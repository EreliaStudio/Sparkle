#include "voxel/model.hpp"

#include <limits>
#include <map>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		using Slots = std::map<std::string, float, std::less<>>;
		using Indices = std::map<std::string, std::uint32_t, std::less<>>;

		void collectSlots(const Model &model, Slots &slots)
		{
			for (const auto &slot : model.materialSlots())
			{
				auto [it, inserted] = slots.try_emplace(slot.name, slot.opacity);
				if (!inserted && it->second != slot.opacity)
				{
					throw spk::Exception("Voxel Model: conflicting slot opacity: " + slot.name);
				}
			}
			for (const auto &[name, anchor] : model.anchors())
			{
				if (anchor.model())
				{
					collectSlots(*anchor.model(), slots);
				}
			}
		}

		[[nodiscard]] Geometry::SurfaceMesh remapSurface(
			const Geometry::SurfaceMesh &source,
			const Geometry &geometry,
			const Indices &indices)
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
				vertex.materialIndex = indices.at(geometry.materialSlot(vertex.materialIndex).name);
				(void)builder.addVertex(vertex);
			}
			for (const auto index : triangles)
			{
				builder.addIndex(index);
			}
			return std::move(builder).build();
		}

		[[nodiscard]] Geometry remapGeometry(
			const Geometry &geometry,
			const std::vector<Definition::Slot> &slots,
			const Indices &indices)
		{
			return Geometry(
				remapSurface(geometry.opaqueMesh(), geometry, indices),
				remapSurface(geometry.transparentMesh(), geometry, indices),
				geometry.depthMesh(),
				slots);
		}

		[[nodiscard]] Model rewrite(
			const Model &source,
			const std::vector<Definition::Slot> &slots,
			const Indices &indices)
		{
			Model::Builder result;
			result.setGeometry(remapGeometry(source.geometry(), slots, indices));
			for (const auto &[name, anchor] : source.anchors())
			{
				result.addAnchor(name, anchor.transform());
				if (anchor.model())
				{
					result.attach(name, rewrite(*anchor.model(), slots, indices), anchor.attachment());
				}
			}
			return result.build();
		}
	}

	void Model::Builder::harmonize()
	{
		const Model source = build();
		Slots collected;
		collectSlots(source, collected);
		if (collected.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw spk::Exception("Voxel Model: too many material slots");
		}
		Indices indices;
		std::vector<Definition::Slot> slots;
		slots.reserve(collected.size());
		for (const auto &[name, opacity] : collected)
		{
			indices.emplace(name, static_cast<std::uint32_t>(slots.size()));
			slots.push_back({name, opacity});
		}
		*this = Builder(rewrite(source, slots, indices));
	}
}
