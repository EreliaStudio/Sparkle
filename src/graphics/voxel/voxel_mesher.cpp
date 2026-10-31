#include "voxel/voxel_mesher.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		using Face = Shape::Face;
		using Polygon = Shape::Polygon;
		using Position = spk::Vector3UInt;
		using SlotIndices = std::map<std::string, std::uint32_t>;

		constexpr std::array<Face, 6> Faces{
			Face::NegativeX,
			Face::PositiveX,
			Face::NegativeY,
			Face::PositiveY,
			Face::NegativeZ,
			Face::PositiveZ};

		[[nodiscard]] Shape::Orientation orientation(const Cell &cell)
		{
			return {cell.horizontalOrientation(), cell.verticalOrientation()};
		}

		[[nodiscard]] std::optional<Position> neighborPosition(Position position, Position dimensions, Face face)
		{
			const std::size_t axis = static_cast<std::size_t>(face) / 2;
			const bool positive = (static_cast<unsigned>(face) & 1U) != 0;
			if (positive)
			{
				if (position.data[axis] == dimensions.data[axis] - 1)
				{
					return std::nullopt;
				}
				++position.data[axis];
			}
			else
			{
				if (position.data[axis] == 0)
				{
					return std::nullopt;
				}
				--position.data[axis];
			}
			return position;
		}

		[[nodiscard]] const Definition &boundaryCube()
		{
			static const Definition result = [] {
				Shape::Builder builder;
				builder.addSlot("boundary");
				builder.addPolygon("boundary", {{{0, 0, 1}}, {{0, 1, 1}}, {{0, 1, 0}}, {{0, 0, 0}}});
				builder.addPolygon("boundary", {{{1, 0, 0}}, {{1, 1, 0}}, {{1, 1, 1}}, {{1, 0, 1}}});
				builder.addPolygon("boundary", {{{0, 0, 0}}, {{1, 0, 0}}, {{1, 0, 1}}, {{0, 0, 1}}});
				builder.addPolygon("boundary", {{{0, 1, 1}}, {{1, 1, 1}}, {{1, 1, 0}}, {{0, 1, 0}}});
				builder.addPolygon("boundary", {{{0, 0, 0}}, {{0, 1, 0}}, {{1, 1, 0}}, {{1, 0, 0}}});
				builder.addPolygon("boundary", {{{1, 0, 1}}, {{1, 1, 1}}, {{0, 1, 1}}, {{0, 0, 1}}});
				return Definition(builder.build(), {{"boundary", {"boundary"}}});
			}();
			return result;
		}

		[[nodiscard]] Occlusion::Result visible(
			const Cell &source,
			const Cell &blocker,
			Face direction,
			float tolerance,
			Occlusion::Cache *cache)
		{
			Occlusion::Cache::Key key;
			key.source = source.definition();
			key.sourceOrientation = orientation(source);
			key.blocker = blocker.definition();
			key.blockerOrientation = orientation(blocker);
			key.direction = direction;
			key.tolerance = tolerance;
			if (cache != nullptr)
			{
				return cache->get(key);
			}
			return Occlusion::Algorithm::compute(
				key.source,
				key.sourceOrientation,
				key.blocker,
				key.blockerOrientation,
				key.direction,
				key.tolerance);
		}

		[[nodiscard]] bool hasInnerShell(const Shape::Instance &instance)
		{
			return std::any_of(instance.polygons().begin(), instance.polygons().end(), [](const Polygon &polygon) {
				return polygon.shell == Shape::Shell::Inner;
			});
		}

		void emit(
			VoxelMesh::Builder &builder,
			const Polygon &polygon,
			Position position,
			std::uint32_t materialIndex,
			float tolerance)
		{
			const auto &vertices = polygon.vertices;
			const spk::Vector3 translation(position);
			for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
			{
				const auto &a = vertices[0];
				const auto &b = vertices[i];
				const auto &c = vertices[i + 1];
				const auto cross = (b.position - a.position).cross(c.position - a.position);
				if (cross.length() <= 2.0 * tolerance * tolerance)
				{
					continue;
				}
				auto add = [&](const Shape::Vertex &vertex) {
					return builder.addVertex({vertex.position + translation, vertex.normal.value_or(polygon.geometricNormal), vertex.uv, materialIndex});
				};
				const auto first = add(a);
				const auto second = add(b);
				const auto third = add(c);
				builder.addTriangle(first, second, third);
			}
		}

		void emit(
			const Polygon &polygon,
			const Definition &definition,
			Position position,
			const SlotIndices &indices,
			VoxelMesh::Builder &opaque,
			VoxelMesh::Builder &translucent,
			float tolerance)
		{
			const auto &slot = definition.slot(polygon.slotKey);
			const auto index = indices.at(slot.name);
			emit(slot.opacity == 1.0f ? opaque : translucent, polygon, position, index, tolerance);
		}

		[[nodiscard]] std::vector<Definition::Slot> collectSlots(const Volume &volume)
		{
			std::map<std::string, float> names;
			const auto dimensions = volume.dimensions();
			for (std::uint32_t z = 0; z < dimensions.z; ++z)
			{
				for (std::uint32_t y = 0; y < dimensions.y; ++y)
				{
					for (std::uint32_t x = 0; x < dimensions.x; ++x)
					{
						for (const auto &[geometricKey, slot] : volume.at({x, y, z}).definition().slots())
						{
							(void)geometricKey;
							auto [it, inserted] = names.try_emplace(slot.name, slot.opacity);
							if (!inserted && it->second != slot.opacity)
							{
								throw spk::Exception("Voxel Model material slot has conflicting opacity: " + slot.name);
							}
						}
					}
				}
			}
			std::vector<Definition::Slot> result;
			for (const auto &[name, opacity] : names)
			{
				result.push_back({name, opacity});
			}
			return result;
		}

		[[nodiscard]] SlotIndices indexSlots(const std::vector<Definition::Slot> &slots)
		{
			SlotIndices result;
			for (std::size_t i = 0; i < slots.size(); ++i)
			{
				if (i > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("Voxel Model material slot index overflow");
				}
				result.emplace(slots[i].name, static_cast<std::uint32_t>(i));
			}
			return result;
		}
	}

	OutsideCell VoxelMesher::outsideCell(Position, Face) const
	{
		return {};
	}

	void VoxelMesher::bind(Occlusion::Cache *cache) noexcept
	{
		_cache = cache;
	}

	void VoxelMesher::setTolerance(float tolerance)
	{
		if (!std::isfinite(tolerance) || tolerance <= 0.0f)
		{
			throw spk::Exception("VoxelMesher tolerance must be positive and finite");
		}
		_tolerance = tolerance;
	}

	float VoxelMesher::tolerance() const noexcept
	{
		return _tolerance;
	}

	Model VoxelMesher::bake(const Volume &volume) const
	{
		const auto slots = collectSlots(volume);
		const auto indices = indexSlots(slots);
		VoxelMesh::Builder opaque;
		VoxelMesh::Builder translucent;
		const auto dimensions = volume.dimensions();
		for (std::uint32_t z = 0; z < dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < dimensions.x; ++x)
				{
					const Position position{x, y, z};
					const Cell cell = volume.at(position);
					if (cell.isAir() || cell.definition().shape().empty())
					{
						continue;
					}
					const auto &instance = cell.definition().shape().instance(orientation(cell));
					bool closed = hasInnerShell(instance);
					for (const auto face : Faces)
					{
						const auto adjacent = neighborPosition(position, dimensions, face);
						const OutsideCell outside = adjacent ? OutsideCell{OutsideCell::State::Occupied, volume.at(*adjacent)} : outsideCell(position, face);
						const bool occupied = outside.state == OutsideCell::State::Occupied && !outside.cell.isAir();
						if (closed && (!occupied || !visible(Cell(boundaryCube()), outside.cell, face, _tolerance, _cache).empty()))
						{
							closed = false;
						}
						if (occupied)
						{
							for (const auto &fragment : visible(cell, outside.cell, face, _tolerance, _cache).fragments())
							{
								emit(fragment, cell.definition(), position, indices, opaque, translucent, _tolerance);
							}
						}
						else
						{
							for (const auto &polygon : instance.polygons())
							{
								if (polygon.shell == Shape::Shell::Outer && polygon.boundary == face)
								{
									emit(polygon, cell.definition(), position, indices, opaque, translucent, _tolerance);
								}
							}
						}
					}
					if (!closed)
					{
						for (const auto &polygon : instance.polygons())
						{
							if (polygon.shell == Shape::Shell::Inner)
							{
								emit(polygon, cell.definition(), position, indices, opaque, translucent, _tolerance);
							}
						}
					}
				}
			}
		}
		VoxelMesh opaqueMesh = std::move(opaque).build();
		auto depth = spk::ShadowMap::Mesh::from(opaqueMesh, [](const MeshVertex &vertex) {
			return vertex.position;
		});
		return Model(std::move(opaqueMesh), std::move(translucent).build(), std::move(depth), slots);
	}
}
