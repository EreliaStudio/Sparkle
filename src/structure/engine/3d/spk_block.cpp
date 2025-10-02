#include "structure/engine/3d/spk_block.hpp"

namespace spk
{
	const std::vector<spk::Vector3> Block::neightbourCoordinates = {{0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 0, -1}};

	spk::SpriteSheet Block::_spriteSheet = spk::SpriteSheet("playground/resources/texture/CubeTexture.png", {9, 1});

	bool Block::Orientation::operator<(const Orientation &p_other) const
	{
		if (horizontalOrientation != p_other.horizontalOrientation)
		{
			return horizontalOrientation < p_other.horizontalOrientation;
		}
		return verticalOrientation < p_other.verticalOrientation;
	}

	const spk::SpriteSheet &Block::spriteSheet()
	{
		return (_spriteSheet);
	}

	spk::SafePointer<const spk::Texture> Block::texture()
	{
		return (&_spriteSheet);
	}

	void Block::_applySprite(spk::ObjMesh::Shape &p_shape, const spk::SpriteSheet::Sprite &p_sprite)
	{
		auto transform = [&](spk::ObjVertex &p_v)
		{
			if (p_v.uv == -1)
			{
				return;
			}
			p_v.uv = (p_v.uv * p_sprite.size) + p_sprite.anchor;
		};

		for (auto &vertex : p_shape)
		{
			transform(vertex);
		}
	}

	bool Block::Cache::hasCase(const Orientation &p_orientation) const
	{
		return _cache.contains(p_orientation);
	}

	void Block::Cache::addCase(const Orientation &p_orientation, const spk::ObjMesh &p_mesh)
	{
		if (hasCase(p_orientation) == false)
		{
			_cache.emplace(p_orientation, _compute(p_mesh, p_orientation));
		}
	}

	const Block::Cache::Entry &Block::Cache::getCase(const Orientation &p_orientation) const
	{
		return _cache.at(p_orientation);
	}

	void Block::Cache::Entry::applyInnerMesh(spk::ObjMesh &p_target, const spk::Vector3 &p_offset) const
	{
		_appendMesh(p_target, innerMesh, p_offset);
	}

	void Block::Cache::Entry::applyFace(spk::ObjMesh &p_target, const spk::Vector3 &p_offset, const spk::Vector3 &p_normal) const
	{
		auto it = faces.find(p_normal);
		if (it != faces.end())
		{
			_appendMesh(p_target, it->second.mesh, p_offset);
		}
	}

	void Block::Cache::Entry::_appendMesh(spk::ObjMesh &p_target, const spk::ObjMesh &p_source, const spk::Vector3 &p_offset)
	{
		for (const auto &shape : p_source.shapes())
		{
			std::vector<spk::ObjVertex> transformed = shape;
			for (auto &vertex : transformed)
			{
				vertex.position += p_offset;
			}
			p_target.addShape(transformed);
		}
	}

	spk::Vector3 Block::Cache::_applyOrientation(const spk::Vector3 &p_position, const Orientation &p_orientation)
	{
		spk::Vector3 result = p_position - spk::Vector3(0.5f, 0.5f, 0.5f);
		constexpr std::array<float, 4> rotations = {-90.0f, 0.0f, 90.0f, 180.0f};
		result = result.rotate({0, rotations[static_cast<size_t>(p_orientation.horizontalOrientation)], 0});

		if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
		{
			result.y = -result.y;
		}

		return result + spk::Vector3(0.5f, 0.5f, 0.5f);
	}

	bool Block::Cache::_isFullQuad(const std::vector<spk::ObjVertex> &p_vertices, const spk::Vector3 &p_normal)
	{
		if (p_vertices.size() != 4)
		{
			return false;
		}

		auto getAB = [&](const spk::Vector3 &p_point) -> std::pair<float, float>
		{
			if (std::abs(p_normal.y) > 0.0f)
			{
				return {p_point.x, p_point.z};
			}
			else if (std::abs(p_normal.x) > 0.0f)
			{
				return {p_point.y, p_point.z};
			}
			else
			{
				return {p_point.x, p_point.y};
			}
		};

		float minA = std::numeric_limits<float>::max();
		float maxA = std::numeric_limits<float>::lowest();
		float minB = std::numeric_limits<float>::max();
		float maxB = std::numeric_limits<float>::lowest();

		for (const auto &v : p_vertices)
		{
			auto [a, b] = getAB(v.position);
			minA = std::min(minA, a);
			maxA = std::max(maxA, a);
			minB = std::min(minB, b);
			maxB = std::max(maxB, b);
		}

		return (
			(std::abs(minA - 0.0f) < spk::Constants::pointPrecision) && (std::abs(maxA - 1.0f) < spk::Constants::pointPrecision) &&
			(std::abs(minB - 0.0f) < spk::Constants::pointPrecision) && (std::abs(maxB - 1.0f) < spk::Constants::pointPrecision));
	}

	void Block::Cache::_applyOrientationToVertices(std::vector<spk::ObjVertex> &p_vertices, const Orientation &p_orientation)
	{
		if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
		{
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::lowest();

			for (const auto &v : p_vertices)
			{
				if (v.uv == -1)
				{
					continue;
				}
				minY = std::min(minY, v.uv.y);
				maxY = std::max(maxY, v.uv.y);
			}
			for (auto &v : p_vertices)
			{
				v.position = _applyOrientation(v.position, p_orientation);
				if (v.uv == -1)
				{
					continue;
				}
				v.uv.y = minY + maxY - v.uv.y;
			}
			std::reverse(p_vertices.begin(), p_vertices.end());
		}
		else
		{
			for (auto &v : p_vertices)
			{
				v.position = _applyOrientation(v.position, p_orientation);
			}
		}
	}

	bool Block::Cache::_isAxisAlignedFace(const std::vector<spk::ObjVertex> &p_vertices, spk::Vector3 &p_outNormal)
	{
		auto eq = [&](float p_a, float p_b) { return std::abs(p_a - p_b) <= spk::Constants::pointPrecision; };

		bool allX0 = true, allX1 = true, allY0 = true, allY1 = true, allZ0 = true, allZ1 = true;
		for (const auto &v : p_vertices)
		{
			allX0 = allX0 && eq(v.position.x, 0.0f);
			allX1 = allX1 && eq(v.position.x, 1.0f);
			allY0 = allY0 && eq(v.position.y, 0.0f);
			allY1 = allY1 && eq(v.position.y, 1.0f);
			allZ0 = allZ0 && eq(v.position.z, 0.0f);
			allZ1 = allZ1 && eq(v.position.z, 1.0f);
		}

		if (allX0 == true)
		{
			p_outNormal = spk::Vector3(-1, 0, 0);
			return true;
		}
		if (allX1 == true)
		{
			p_outNormal = spk::Vector3(1, 0, 0);
			return true;
		}
		if (allY0 == true)
		{
			p_outNormal = spk::Vector3(0, -1, 0);
			return true;
		}
		if (allY1 == true)
		{
			p_outNormal = spk::Vector3(0, 1, 0);
			return true;
		}
		if (allZ0 == true)
		{
			p_outNormal = spk::Vector3(0, 0, -1);
			return true;
		}
		if (allZ1 == true)
		{
			p_outNormal = spk::Vector3(0, 0, 1);
			return true;
		}

		return false;
	}

	void Block::Cache::_addVerticesToMesh(spk::ObjMesh &p_mesh, const std::vector<spk::ObjVertex> &p_vertices)
	{
		if (p_vertices.size() == 3)
		{
			p_mesh.addShape(p_vertices[0], p_vertices[1], p_vertices[2]);
		}
		else
		{
			p_mesh.addShape(p_vertices[0], p_vertices[1], p_vertices[2], p_vertices[3]);
		}
	}

	Block::Cache::Entry Block::Cache::_compute(const spk::ObjMesh &p_mesh, const Orientation &p_orientation)
	{
		Entry result;

		size_t shapeIndex = 0;
		for (const auto &baseShape: p_mesh.shapes())
		{
			auto shape = baseShape;
			_applyOrientationToVertices(shape, p_orientation);

			spk::Vector3 normal;
			if (_isAxisAlignedFace(shape, normal) == true)
			{
				Face &face = result.faces[normal];
				std::vector<spk::Vector3> footprintPoints;
				footprintPoints.reserve(shape.size());
				for (const auto &v : shape)
				{
					footprintPoints.push_back(v.position);
				}
				spk::Polygon footprint = spk::Polygon::fromLoop(footprintPoints);
				if (face.footprint.points().empty() == true)
				{
					face.footprint = footprint;
				}
				else
				{
					try
					{
						face.footprint = face.footprint.fuze(footprint);
					} catch (const std::exception &e)
					{
						PROPAGATE_ERROR("Error while merging footprint for face " + std::to_string(shapeIndex), e);
					}
				}

				_addVerticesToMesh(face.mesh, shape);

				face.full = face.full == true || (shape.size() == 4 && _isFullQuad(shape, normal) == true);
			}
			else
			{
				_addVerticesToMesh(result.innerMesh, shape);
			}
		}

		return result;
	}

	const Block::Cache::Entry &Block::_ensureCacheCase(const Orientation &p_orientation) const
	{
		if (_cache.hasCase(p_orientation) == false)
		{
			_cache.addCase(p_orientation, _mesh());
		}
		return _cache.getCase(p_orientation);
	}

	Block::NeighFaces Block::_gatherNeighbourFaces(const NeightbourDescriber &p_neightbourDescriber) const
	{
		NeighFaces neighFaces{};
		for (size_t i = 0; i < 6; ++i)
		{
			const auto &desc = p_neightbourDescriber[i];
			if (desc.first == nullptr)
			{
				continue;
			}

			const Block *neigh = desc.first;
			const Orientation &neighOrientation = desc.second;

			if (neigh->_cache.hasCase(neighOrientation) == false)
			{
				neigh->_cache.addCase(neighOrientation, neigh->_mesh());
			}

			const Cache::Entry &neighData = neigh->_cache.getCase(neighOrientation);
			const spk::Vector3 oppositeNormal = -neightbourCoordinates[i];
			for (const auto &[normal, face] : neighData.faces)
			{
				if (normal.dot(oppositeNormal) > 0)
				{
					neighFaces[i].push_back(&face);
				}
			}
		}
		return neighFaces;
	}

	bool Block::_allSixFull(const NeighFaces &p_neighFaces)
	{
		for (const auto &faces : p_neighFaces)
		{
			bool sideFull = false;
			for (const Face *nf : faces)
			{
				if (nf->full == true)
				{
					sideFull = true;
					break;
				}
			}
			if (sideFull == false)
			{
				return false;
			}
		}
		return true;
	}

	void Block::_emitInnerIfNeeded(spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const NeighFaces &p_neighFaces)
	{
		if (_allSixFull(p_neighFaces) == false)
		{
			p_data.applyInnerMesh(p_toFill, p_position);
		}
	}

	spk::Polygon Block::_translated(const spk::Polygon &p_poly, const spk::Vector3 &p_delta)
	{
		const std::vector<spk::Vector3> &pts = p_poly.points();
		std::vector<spk::Vector3> translated;
		translated.reserve(pts.size());
		std::transform(pts.begin(), pts.end(), std::back_inserter(translated), [&](const spk::Vector3 &p_point) { return p_point + p_delta; });
		return spk::Polygon::fromLoop(translated);
	}

	void Block::_emitVisibleFaces(spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const NeighFaces &p_neighFaces)
	{
		try
		{
			for (size_t i = 0; i < 6; ++i)
			{
				const spk::Vector3 normal = neightbourCoordinates[i];
				const Face *ourFace = nullptr;

				try
				{
					auto faceIt = p_data.faces.find(normal);
					if (faceIt == p_data.faces.end())
					{
						continue;
					}

					ourFace = &faceIt->second;
					if (ourFace->mesh.shapes().empty() == true)
					{
						continue;
					}
				} catch (const std::exception &e)
				{
					PROPAGATE_ERROR("Error while preparing face data for visibility evaluation", e);
				}

				bool visible = true;

				try
				{
					const spk::Vector3 toOurLocal = normal;
					for (const Face *neigh : p_neighFaces[i])
					{
						if (neigh->footprint.points().empty() == true)
						{
							continue;
						}

						spk::Polygon neighInOurSpace = _translated(neigh->footprint, toOurLocal);

						if (neighInOurSpace.contains(ourFace->footprint) == true)
						{
							visible = false;
							break;
						}
					}
				} catch (const std::exception &e)
				{
					PROPAGATE_ERROR("Error while evaluating visibility against neighbour face", e);
				}

				try
				{
					if (visible == true)
					{
						p_data.applyFace(p_toFill, p_position, normal);
					}
				} catch (const std::exception &e)
				{
					PROPAGATE_ERROR("Error while applying a visible face to the mesh", e);
				}
			}
		} catch (const std::exception &e)
		{
			PROPAGATE_ERROR("Error while emitting visible faces", e);
		}
	}

	void Block::bake(
		spk::ObjMesh &p_toFill,
		const NeightbourDescriber &p_neightbourDescriber,
		const spk::Vector3 &p_position,
		const Orientation &p_orientation) const
	{
		const Cache::Entry &data = _ensureCacheCase(p_orientation);

		NeighFaces neighFaces;

		try
		{
			neighFaces = _gatherNeighbourFaces(p_neightbourDescriber);
		} catch (const std::exception &e)
		{
			PROPAGATE_ERROR("Error while gathering neighbour faces", e);
		}

		try
		{
			_emitInnerIfNeeded(p_toFill, data, p_position, neighFaces);
		} catch (const std::exception &e)
		{
			PROPAGATE_ERROR("Error while emitting inner mesh", e);
		}

		try
		{
			_emitVisibleFaces(p_toFill, data, p_position, neighFaces);
		} catch (const std::exception &e)
		{
			PROPAGATE_ERROR("Error while emitting visible faces", e);
		}
	}
} // namespace spk
