#include "voxel/mesher.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
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
		using SlotIndexCollection = std::map<std::string, std::uint32_t>;
		using MaterialOpacityCollection = std::map<std::string, float>;
		struct ResolvedSlot
		{
			std::string geometricKey;
			std::uint32_t materialIndex;
			bool opaque;
		};
		using ResolvedSlotContainer = std::vector<ResolvedSlot>;
		using MeshVertexContainer = std::vector<Geometry::Vertex>;

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

		[[nodiscard]] std::optional<std::size_t> neighborIndex(Position position, Position dimensions, std::size_t index, const std::array<std::size_t, 3> &strides, Face face)
		{
			const std::size_t axis = static_cast<std::size_t>(face) / 2;
			const bool positive = (static_cast<unsigned>(face) & 1U) != 0;
			if (positive)
			{
				if (position.data[axis] == dimensions.data[axis] - 1)
				{
					return std::nullopt;
				}
				return index + strides[axis];
			}
			if (position.data[axis] == 0)
			{
				return std::nullopt;
			}
			return index - strides[axis];
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

		void emit(
			MeshVertexContainer &verticesOut,
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
				verticesOut.push_back({a.position + translation, a.normal.value_or(polygon.geometricNormal), a.uv, materialIndex});
				verticesOut.push_back({b.position + translation, b.normal.value_or(polygon.geometricNormal), b.uv, materialIndex});
				verticesOut.push_back({c.position + translation, c.normal.value_or(polygon.geometricNormal), c.uv, materialIndex});
			}
		}

		void emit(
			const Polygon &polygon,
			Position position,
			const ResolvedSlotContainer &slots,
			MeshVertexContainer &opaque,
			MeshVertexContainer &translucent,
			float tolerance)
		{
			const auto entry = std::find_if(slots.begin(), slots.end(), [&](const ResolvedSlot &slot) {
				return slot.geometricKey == polygon.slotKey;
			});
			if (entry == slots.end())
			{
				throw spk::Exception("Voxel Definition references an unmapped geometric slot");
			}
			emit(entry->opaque ? opaque : translucent, polygon, position, entry->materialIndex, tolerance);
		}

		struct SurfaceTriangles
		{
			MeshVertexContainer opaque;
			MeshVertexContainer translucent;
		};

		struct CellTemplate
		{
			Cell cell;
			std::array<SurfaceTriangles, 6> outer;
			SurfaceTriangles inner;
			std::optional<SurfaceTriangles> isolated;
			bool hasInner = false;
			std::array<bool, 6> hasOuter{};
			std::array<std::optional<bool>, 6> fullyOccludes{};
		};

		using OrientedCellTemplateContainer = std::array<std::optional<CellTemplate>, 8>;
		using CellTemplateContainer = std::vector<OrientedCellTemplateContainer>;

		[[nodiscard]] CellTemplate buildCellTemplate(
			const Shape::Instance &instance,
			const ResolvedSlotContainer &slots,
			float tolerance)
		{
			CellTemplate result;
			for (const Polygon &polygon : instance.triangles())
			{
				SurfaceTriangles *surface;
				if (polygon.shell == Shape::Shell::Inner)
				{
					result.hasInner = true;
					surface = &result.inner;
				}
				else if (polygon.boundary)
				{
					const auto faceIndex = static_cast<std::size_t>(*polygon.boundary);
					result.hasOuter[faceIndex] = true;
					surface = &result.outer[faceIndex];
				}
				else
				{
					continue;
				}
				emit(polygon, {0, 0, 0}, slots, surface->opaque, surface->translucent, tolerance);
			}
			return result;
		}

		[[nodiscard]] const SurfaceTriangles &isolatedTriangles(CellTemplate &entry)
		{
			if (entry.isolated.has_value())
			{
				return *entry.isolated;
			}
			auto &result = entry.isolated.emplace();
			std::size_t opaqueCount = entry.inner.opaque.size();
			std::size_t translucentCount = entry.inner.translucent.size();
			for (const auto &face : entry.outer)
			{
				opaqueCount += face.opaque.size();
				translucentCount += face.translucent.size();
			}
			result.opaque.reserve(opaqueCount);
			result.translucent.reserve(translucentCount);
			for (const auto &face : entry.outer)
			{
				result.opaque.insert(result.opaque.end(), face.opaque.begin(), face.opaque.end());
				result.translucent.insert(result.translucent.end(), face.translucent.begin(), face.translucent.end());
			}
			result.opaque.insert(result.opaque.end(), entry.inner.opaque.begin(), entry.inner.opaque.end());
			result.translucent.insert(result.translucent.end(), entry.inner.translucent.begin(), entry.inner.translucent.end());
			return result;
		}

		void appendTranslated(MeshVertexContainer &output, const MeshVertexContainer &input, Position position)
		{
			if (input.empty())
			{
				return;
			}
			const auto offset = output.size();
			output.insert(output.end(), input.begin(), input.end());
			const spk::Vector3 translation(position);
			for (std::size_t i = offset; i < output.size(); ++i)
			{
				output[i].position += translation;
			}
		}

		void appendTranslated(MeshVertexContainer &opaque, MeshVertexContainer &translucent, const SurfaceTriangles &triangles, Position position)
		{
			appendTranslated(opaque, triangles.opaque, position);
			appendTranslated(translucent, triangles.translucent, position);
		}

		[[nodiscard]] std::size_t orientationIndex(Shape::Orientation value)
		{
			return (static_cast<unsigned>(value.vertical) * 4) + static_cast<unsigned>(value.horizontal) / 90;
		}

		void emit(
			const Occlusion::Result::Triangle &triangle,
			Position position,
			const ResolvedSlotContainer &slots,
			MeshVertexContainer &opaque,
			MeshVertexContainer &translucent,
			float tolerance)
		{
			if (triangle.crossLength <= 2.0 * tolerance * tolerance)
			{
				return;
			}
			const ResolvedSlot *entry = nullptr;
			if (triangle.slotIndex < slots.size())
			{
				entry = &slots[triangle.slotIndex];
			}
			else
			{
				const auto found = std::find_if(slots.begin(), slots.end(), [&](const ResolvedSlot &slot) {
					return slot.geometricKey == triangle.slotKey;
				});
				if (found == slots.end())
				{
					throw spk::Exception("Voxel Definition references an unmapped geometric slot");
				}
				entry = &*found;
			}
			auto &output = entry->opaque ? opaque : translucent;
			const spk::Vector3 translation(position);
			for (const auto &vertex : triangle.vertices)
			{
				output.push_back({vertex.position + translation, vertex.normal, vertex.uv, entry->materialIndex});
			}
		}

		[[nodiscard]] Geometry::SurfaceMesh buildSurface(const MeshVertexContainer &vertices)
		{
			Geometry::SurfaceMesh::Builder builder;
			if (!vertices.empty())
			{
				(void)builder.addVertices(vertices);
				builder.addSequentialIndices(vertices.size());
			}
			return std::move(builder).build();
		}

		struct PreparedVolume
		{
			std::span<const Volume::PackedCell::Storage> cells;
			std::size_t occupied = 0;
			std::vector<std::size_t> definitionIndices;
			std::vector<Definition> definitions;
			std::vector<Definition::Slot> slots;
		};

		[[nodiscard]] PreparedVolume prepare(const Volume &volume)
		{
			PreparedVolume result;
			const auto dimensions = volume.dimensions();
			const auto count = static_cast<std::size_t>(dimensions.x) * dimensions.y * dimensions.z;
			result.cells = volume.packedCells();
			result.definitionIndices.reserve(count);
			MaterialOpacityCollection names;
			std::vector<std::pair<Definition::Registry::ID, std::size_t>> registeredDefinitions;
			std::vector<bool> hasGeometry;
			std::size_t cellIndex = 0;
			for (std::uint32_t z = 0; z < dimensions.z; ++z)
			{
				for (std::uint32_t y = 0; y < dimensions.y; ++y)
				{
					for (std::uint32_t x = 0; x < dimensions.x; ++x)
					{
						const auto packed = result.cells[cellIndex++];
						const auto id = Volume::PackedCell::definitionId(packed);
						const auto found = std::find_if(registeredDefinitions.begin(), registeredDefinitions.end(), [id](const auto &entry) {
							return entry.first == id;
						});
						std::size_t definitionIndex;
						if (found == registeredDefinitions.end())
						{
							const auto cell = volume.at({x, y, z});
							const auto &definition = cell.definition();
							const auto existing = std::find(result.definitions.begin(), result.definitions.end(), definition);
							definitionIndex = static_cast<std::size_t>(existing - result.definitions.begin());
							registeredDefinitions.emplace_back(id, definitionIndex);
							if (existing == result.definitions.end())
							{
								result.definitions.push_back(definition);
								hasGeometry.push_back(!definition.isAir() && !definition.shape().empty());
								for (const auto &[geometricKey, slot] : definition.slots())
								{
									(void)geometricKey;
									auto [it, inserted] = names.try_emplace(slot.name, slot.opacity);
									if (!inserted && it->second != slot.opacity)
									{
										throw spk::Exception("Voxel Geometry material slot has conflicting opacity: " + slot.name);
									}
								}
							}
						}
						else
						{
							definitionIndex = found->second;
						}
						result.occupied += hasGeometry[definitionIndex];
						result.definitionIndices.push_back(definitionIndex);
					}
				}
			}
			result.slots.reserve(names.size());
			for (const auto &[name, opacity] : names)
			{
				result.slots.push_back({name, opacity});
			}
			return result;
		}

		[[nodiscard]] SlotIndexCollection indexSlots(const std::vector<Definition::Slot> &slots)
		{
			SlotIndexCollection result;
			for (std::size_t i = 0; i < slots.size(); ++i)
			{
				if (i > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("Voxel Mesher: too many material slots");
				}
				result.emplace(slots[i].name, static_cast<std::uint32_t>(i));
			}
			return result;
		}
	}

	Mesher::OutsideCell Mesher::outsideCell(Position, Face) const
	{
		return {};
	}

	void Mesher::attachCache(Occlusion::Cache &cache) noexcept
	{
		_cache = &cache;
	}

	void Mesher::detachCache() noexcept
	{
		_cache = nullptr;
	}

	void Mesher::setTolerance(float tolerance)
	{
		if (!std::isfinite(tolerance) || tolerance <= 0.0f)
		{
			throw spk::Exception("Mesher tolerance must be positive and finite");
		}
		_tolerance = tolerance;
	}

	float Mesher::tolerance() const noexcept
	{
		return _tolerance;
	}

	Geometry Mesher::bake(const Volume &volume) const
	{
		auto prepared = prepare(volume);
		const auto indices = indexSlots(prepared.slots);
		std::vector<ResolvedSlotContainer> materials;
		materials.reserve(prepared.definitions.size());
		for (const auto &definition : prepared.definitions)
		{
			ResolvedSlotContainer resolved;
			resolved.reserve(definition.slots().size());
			for (const auto &[geometricKey, slot] : definition.slots())
			{
				resolved.push_back({geometricKey, indices.at(slot.name), slot.opacity == 1.0f});
			}
			materials.push_back(std::move(resolved));
		}
		MeshVertexContainer opaque;
		opaque.reserve(std::min<std::size_t>(prepared.occupied, 2500) * 40);
		MeshVertexContainer translucent;
		const auto dimensions = volume.dimensions();
		CellTemplateContainer templates(prepared.definitions.size());
		const std::array<std::size_t, 3> strides{1, dimensions.x, static_cast<std::size_t>(dimensions.x) * dimensions.y};
		std::vector<CellTemplate *> cellTemplates(prepared.cells.size(), nullptr);
		for (std::size_t index = 0; index < prepared.cells.size(); ++index)
		{
			const auto id = prepared.definitionIndices[index];
			const auto &definition = prepared.definitions[id];
			if (definition.isAir() || definition.shape().empty())
			{
				continue;
			}
			const auto packed = prepared.cells[index];
			const Shape::Orientation orientationValue{Volume::PackedCell::horizontal(packed), Volume::PackedCell::vertical(packed)};
			auto &entry = templates[id][orientationIndex(orientationValue)];
			if (!entry)
			{
				entry.emplace(buildCellTemplate(definition.shape().instance(orientationValue), materials[id], _tolerance));
				entry->cell = Cell(definition, orientationValue.horizontal, orientationValue.vertical);
			}
			cellTemplates[index] = &*entry;
		}
		for (std::uint32_t z = 0; z < dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < dimensions.x; ++x)
				{
					const Position position{x, y, z};
					const auto cellIndex = (static_cast<std::size_t>(z) * dimensions.y + y) * dimensions.x + x;
					if (cellTemplates[cellIndex] == nullptr)
					{
						continue;
					}
					auto &faces = *cellTemplates[cellIndex];
					// Interior cells without geometrically occupied neighbors cannot
					// have clipped faces or a closed inner shell. Emit their complete
					// surface in one batch, preserving face and material order.
					if (x > 0 && x + 1 < dimensions.x &&
						y > 0 && y + 1 < dimensions.y &&
						z > 0 && z + 1 < dimensions.z &&
						cellTemplates[cellIndex - strides[0]] == nullptr &&
						cellTemplates[cellIndex + strides[0]] == nullptr &&
						cellTemplates[cellIndex - strides[1]] == nullptr &&
						cellTemplates[cellIndex + strides[1]] == nullptr &&
						cellTemplates[cellIndex - strides[2]] == nullptr &&
						cellTemplates[cellIndex + strides[2]] == nullptr)
					{
						appendTranslated(opaque, translucent, isolatedTriangles(faces), position);
						continue;
					}
					const auto &resolved = materials[prepared.definitionIndices[cellIndex]];
					const Cell &cell = faces.cell;
					bool closed = faces.hasInner;
					for (const auto face : Faces)
					{
						const auto faceIndex = static_cast<std::size_t>(face);
						const auto &faceTriangles = faces.outer[faceIndex];
						const bool hasFace = !faceTriangles.opaque.empty() || !faceTriangles.translucent.empty();
						if (!closed && !hasFace)
						{
							continue;
						}
						const auto adjacent = neighborIndex(position, dimensions, cellIndex, strides, face);
						std::optional<OutsideCell> outside;
						const Cell *blocker;
						std::optional<std::size_t> blockerIndex;
						bool occupied;
						if (adjacent)
						{
							blockerIndex = *adjacent;
							occupied = cellTemplates[*blockerIndex] != nullptr;
							blocker = occupied ? &cellTemplates[*blockerIndex]->cell : nullptr;
						}
						else
						{
							outside.emplace(outsideCell(position, face));
							blocker = &outside->cell;
							occupied = outside->state == OutsideCell::State::Occupied && !blocker->isAir();
						}
						CellTemplate *blockerTemplates = occupied && blockerIndex ? cellTemplates[*blockerIndex] : nullptr;
						// Without an opposing boundary polygon, this neighbor cannot clip
						// the source face. Reuse the prepared triangles unchanged.
						const bool blockerCanOcclude = blockerTemplates == nullptr || blockerTemplates->hasOuter[faceIndex ^ 1U];
						bool fullyOccluded = false;
						if (occupied && blockerCanOcclude && (closed || hasFace))
						{
							if (blockerTemplates != nullptr)
							{
								auto &cached = blockerTemplates->fullyOccludes[faceIndex];
								if (!cached)
								{
									cached = visible(Cell(boundaryCube()), *blocker, face, _tolerance, _cache).empty();
								}
								fullyOccluded = *cached;
							}
							else
							{
								fullyOccluded = visible(Cell(boundaryCube()), *blocker, face, _tolerance, _cache).empty();
							}
						}
						if (closed && (!occupied || !fullyOccluded))
						{
							closed = false;
						}
						if (occupied)
						{
							if (hasFace && !fullyOccluded)
							{
								if (!blockerCanOcclude)
								{
									appendTranslated(opaque, translucent, faceTriangles, position);
								}
								else
								{
									const auto visibility = visible(cell, *blocker, face, _tolerance, _cache);
									for (const auto &triangle : visibility.triangles())
									{
										emit(triangle, position, resolved, opaque, translucent, _tolerance);
									}
								}
							}
						}
						else
						{
							appendTranslated(opaque, translucent, faceTriangles, position);
						}
					}
					if (!closed)
					{
						appendTranslated(opaque, translucent, faces.inner, position);
					}
				}
			}
		}
		Geometry::SurfaceMesh opaqueMesh = buildSurface(opaque);
		auto depth = spk::ShadowMap::Mesh::from(opaqueMesh, [](const Geometry::Vertex &vertex) {
			return vertex.position;
		});
		return Geometry(std::move(opaqueMesh), buildSurface(translucent), std::move(depth), std::move(prepared.slots));
	}
}
