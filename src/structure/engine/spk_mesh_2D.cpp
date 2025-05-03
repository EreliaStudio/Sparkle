#include "structure/engine/spk_mesh_2D.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Mesh2D::VertexIndex::VertexIndex() :
		pointIndex(),
		uvIndex()
	{
	}

	Mesh2D::VertexIndex::VertexIndex(size_t p_pointIndex, size_t p_uvIndex) :
		pointIndex(p_pointIndex),
		uvIndex(p_uvIndex)
	{
	}

	bool Mesh2D::VertexIndex::operator<(const VertexIndex &p_other) const
	{
		return std::tie(pointIndex, uvIndex) < std::tie(p_other.pointIndex, p_other.uvIndex);
	}

	bool Mesh2D::VertexIndex::operator==(const VertexIndex &p_other) const
	{
		return (pointIndex == p_other.pointIndex) && (uvIndex == p_other.uvIndex);
	}

	bool Mesh2D::VertexIndex::operator!=(const VertexIndex &p_other) const
	{
		return !(*this == p_other);
	}

	Mesh2D::Mesh2D()
	{
	}

	size_t Mesh2D::addVertex(const Mesh2D::VertexIndex &p_vertex)
	{
		auto it = std::find(_vertexIndexes.begin(), _vertexIndexes.end(), p_vertex);
		if (it != _vertexIndexes.end())
		{
			return std::distance(_vertexIndexes.begin(), it);
		}
		else
		{
			_vertexIndexes.push_back(p_vertex);
			return _vertexIndexes.size() - 1;
		}
	}

	size_t Mesh2D::addPoint(const spk::Vector2 &p_point)
	{
		auto it = std::find(_points.begin(), _points.end(), p_point);
		if (it != _points.end())
		{
			return std::distance(_points.begin(), it);
		}
		else
		{
			_points.push_back(p_point);
			return _points.size() - 1;
		}
	}

	size_t Mesh2D::addUV(const spk::Vector2 &p_uv)
	{
		auto it = std::find(_UVs.begin(), _UVs.end(), p_uv);
		if (it != _UVs.end())
		{
			return std::distance(_UVs.begin(), it);
		}
		else
		{
			_UVs.push_back(p_uv);
			return _UVs.size() - 1;
		}
	}

	void Mesh2D::addFace(const VertexIndex &p_vertexA, const VertexIndex &p_vertexB, const VertexIndex &p_vertexC)
	{
		if (!p_vertexA.pointIndex.has_value() || p_vertexA.pointIndex.value() >= _points.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid pointIndex in VertexA");
		}
		if (!p_vertexB.pointIndex.has_value() || p_vertexB.pointIndex.value() >= _points.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid pointIndex in VertexB");
		}
		if (!p_vertexC.pointIndex.has_value() || p_vertexC.pointIndex.value() >= _points.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid pointIndex in VertexC");
		}

		if (p_vertexA.uvIndex.has_value() && p_vertexA.uvIndex.value() >= _UVs.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid uvIndex in VertexA");
		}
		if (p_vertexB.uvIndex.has_value() && p_vertexB.uvIndex.value() >= _UVs.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid uvIndex in VertexB");
		}
		if (p_vertexC.uvIndex.has_value() && p_vertexC.uvIndex.value() >= _UVs.size())
		{
			GENERATE_ERROR("Mesh2D::addFace: Invalid uvIndex in VertexC");
		}

		_indexes.push_back(static_cast<unsigned int>(addVertex(p_vertexA)));
		_indexes.push_back(static_cast<unsigned int>(addVertex(p_vertexB)));
		_indexes.push_back(static_cast<unsigned int>(addVertex(p_vertexC)));
	}

	void Mesh2D::addFace(const VertexIndex &p_vertexA, const VertexIndex &p_vertexB, const VertexIndex &p_vertexC, const VertexIndex &p_vertexD)
	{
		addFace(p_vertexA, p_vertexB, p_vertexC);
		addFace(p_vertexC, p_vertexB, p_vertexD);
	}

	void Mesh2D::validate()
	{
		_vertices.clear();
		_vertices.reserve(_vertexIndexes.size());

		for (const auto &vertexIndex : _vertexIndexes)
		{
			if (!vertexIndex.pointIndex.has_value())
			{
				GENERATE_ERROR("Mesh2D::bake: VertexIndex has no pointIndex set.");
			}
			size_t pIdx = vertexIndex.pointIndex.value();

			spk::Vector2 uv(0.0f, 0.0f);
			if (vertexIndex.uvIndex.has_value())
			{
				size_t uvIdx = vertexIndex.uvIndex.value();
				if (uvIdx >= _UVs.size())
				{
					GENERATE_ERROR("Mesh2D::bake: VertexIndex uvIndex out of range.");
				}
				uv = _UVs[uvIdx];
			}

			if (pIdx >= _points.size())
			{
				GENERATE_ERROR("Mesh2D::bake: VertexIndex pointIndex out of range.");
			}

			_vertices.push_back({_points[pIdx], uv});
		}

		_onValidationContractProvider.trigger();
	}

	Mesh2D::Contract Mesh2D::subscribeToValidation(const Job &p_job)
	{
		return _onValidationContractProvider.subscribe(p_job);
	}

	const std::vector<spk::Vector2> &Mesh2D::points() const
	{
		return _points;
	}

	const std::vector<spk::Vector2> &Mesh2D::UVs() const
	{
		return _UVs;
	}

	std::vector<Mesh2D::Vertex> &Mesh2D::vertices()
	{
		return _vertices;
	}

	const std::vector<Mesh2D::Vertex> &Mesh2D::vertices() const
	{
		return _vertices;
	}

	std::vector<unsigned int> &Mesh2D::indexes()
	{
		return _indexes;
	}

	const std::vector<unsigned int> &Mesh2D::indexes() const
	{
		return _indexes;
	}
}