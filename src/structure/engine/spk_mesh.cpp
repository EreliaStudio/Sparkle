#include "structure/engine/spk_mesh.hpp"

#include "spk_debug_macro.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Mesh::VertexIndex::VertexIndex() :
		pointIndex(),
		uvIndex(),
		normalIndex()
	{
	}

	Mesh::VertexIndex::VertexIndex(size_t p_pointIndex, size_t p_uvIndex) :
		pointIndex(p_pointIndex),
		uvIndex(p_uvIndex),
		normalIndex()
	{
	}

	Mesh::VertexIndex::VertexIndex(size_t p_pointIndex, size_t p_uvIndex, size_t p_normalIndex) :
		pointIndex(p_pointIndex),
		uvIndex(p_uvIndex),
		normalIndex(p_normalIndex)
	{
	}

	bool Mesh::VertexIndex::operator<(const VertexIndex &p_other) const
	{
		return (std::tie(pointIndex, uvIndex, normalIndex) < std::tie(p_other.pointIndex, p_other.uvIndex, p_other.normalIndex));
	}

	bool Mesh::VertexIndex::operator==(const VertexIndex &p_other) const
	{
		return (pointIndex == p_other.pointIndex) && (uvIndex == p_other.uvIndex) && (normalIndex == p_other.normalIndex);
	}

	bool Mesh::VertexIndex::operator!=(const VertexIndex &p_other) const
	{
		return (!(*this == p_other));
	}

	size_t Mesh::addVertex(VertexIndex p_vertex)
	{
		auto it = std::find(_vertexIndexes.begin(), _vertexIndexes.end(), p_vertex);
		if (it != _vertexIndexes.end())
		{
			return (std::distance(_vertexIndexes.begin(), it));
		}
		else
		{
			_vertexIndexes.push_back(p_vertex);
			return (_vertexIndexes.size() - 1);
		}
	}

	Mesh::Mesh()
	{
	}

	size_t Mesh::addPoint(const spk::Vector3 &p_point)
	{
		auto it = std::find(_points.begin(), _points.end(), p_point);
		if (it != _points.end())
		{
			return (std::distance(_points.begin(), it));
		}
		else
		{
			_points.push_back(p_point);
			return (_points.size() - 1);
		}
	}

	size_t Mesh::addUV(const spk::Vector2 &p_uv)
	{
		auto it = std::find(_UVs.begin(), _UVs.end(), p_uv);
		if (it != _UVs.end())
		{
			return (std::distance(_UVs.begin(), it));
		}
		else
		{
			_UVs.push_back(p_uv);
			return (_UVs.size() - 1);
		}
	}

	size_t Mesh::addNormal(const spk::Vector3 &p_normal)
	{
		auto it = std::find(_normals.begin(), _normals.end(), p_normal);
		if (it != _normals.end())
		{
			return (std::distance(_normals.begin(), it));
		}
		else
		{
			_normals.push_back(p_normal);
			return (_normals.size() - 1);
		}
	}

	void Mesh::addFace(Mesh::VertexIndex p_vertexA, Mesh::VertexIndex p_vertexB, Mesh::VertexIndex p_vertexC)
	{
		if (p_vertexA.pointIndex.has_value() == false || _points.size() <= p_vertexA.pointIndex.value())
		{
			GENERATE_ERROR("Error loading a face with invalid index [" +
						   (p_vertexA.pointIndex.has_value() == true ? std::to_string(p_vertexA.pointIndex.value()) : "No value") + "] over [" +
						   std::to_string(_points.size()) + "] points");
		}
		if (p_vertexB.pointIndex.has_value() == false || _points.size() <= p_vertexB.pointIndex.value())
		{
			GENERATE_ERROR("Error loading a face with invalid index [" +
						   (p_vertexB.pointIndex.has_value() == true ? std::to_string(p_vertexB.pointIndex.value()) : "No value") + "] over [" +
						   std::to_string(_points.size()) + "] points");
		}
		if (p_vertexC.pointIndex.has_value() == false || _points.size() <= p_vertexC.pointIndex.value())
		{
			GENERATE_ERROR("Error loading a face with invalid index [" +
						   (p_vertexC.pointIndex.has_value() == true ? std::to_string(p_vertexC.pointIndex.value()) : "No value") + "] over [" +
						   std::to_string(_points.size()) + "] points");
		}

		if (p_vertexA.normalIndex.has_value() == false || p_vertexB.normalIndex.has_value() == false || p_vertexC.normalIndex.has_value() == false)
		{
			spk::Vector3 pointA = _points[p_vertexA.pointIndex.value()];
			spk::Vector3 pointB = _points[p_vertexB.pointIndex.value()];
			spk::Vector3 pointC = _points[p_vertexC.pointIndex.value()];

			spk::Vector3 newNormal = (pointC - pointA).cross(pointB - pointA);

			size_t value = addNormal(newNormal);

			if (p_vertexA.normalIndex.has_value() == false)
			{
				p_vertexA.normalIndex = value;
			}

			if (p_vertexB.normalIndex.has_value() == false)
			{
				p_vertexB.normalIndex = value;
			}

			if (p_vertexC.normalIndex.has_value() == false)
			{
				p_vertexC.normalIndex = value;
			}
		}

		_indexes.push_back(addVertex(p_vertexA));
		_indexes.push_back(addVertex(p_vertexB));
		_indexes.push_back(addVertex(p_vertexC));
	}

	void Mesh::addFace(Mesh::VertexIndex p_vertexA, Mesh::VertexIndex p_vertexB, Mesh::VertexIndex p_vertexC, Mesh::VertexIndex p_vertexD)
	{
		addFace(p_vertexA, p_vertexB, p_vertexC);
		addFace(p_vertexC, p_vertexB, p_vertexD);
	}

	void Mesh::validate()
	{
		_vertices.clear();

		for (const auto &vertexIndex : _vertexIndexes)
		{
			if (vertexIndex.pointIndex.has_value() == false || vertexIndex.uvIndex.has_value() == false ||
				vertexIndex.normalIndex.has_value() == false)
			{
				std::cout << "Point  [" << (vertexIndex.pointIndex.has_value() == true ? std::to_string(vertexIndex.pointIndex.value()) : "Not set")
						  << "]" << std::endl;
				std::cout << "UVs    [" << (vertexIndex.uvIndex.has_value() == true ? std::to_string(vertexIndex.uvIndex.value()) : "Not set") << "]"
						  << std::endl;
				std::cout << "Normal [" << (vertexIndex.normalIndex.has_value() == true ? std::to_string(vertexIndex.normalIndex.value()) : "Not set")
						  << "]" << std::endl;
				GENERATE_ERROR("Error: VertexIndex contains unset optional values. Ensure all indices are set before baking.");
			}

			_vertices.push_back({.point = _points[vertexIndex.pointIndex.value()],
								 .uv = _UVs[vertexIndex.uvIndex.value()],
								 .normal = _normals[vertexIndex.normalIndex.value()]});
		}

		_onValidationContractProvider.trigger();
	}

	Mesh::Contract Mesh::subscribeToValidation(const Job &p_job)
	{
		return (_onValidationContractProvider.subscribe(p_job));
	}

	const std::vector<spk::Vector3> &Mesh::points() const
	{
		return (_points);
	}

	const std::vector<spk::Vector2> &Mesh::UVs() const
	{
		return (_UVs);
	}

	const std::vector<spk::Vector3> &Mesh::normals() const
	{
		return (_normals);
	}

	std::vector<Mesh::Vertex> &Mesh::vertices()
	{
		return (_vertices);
	}

	const std::vector<Mesh::Vertex> &Mesh::vertices() const
	{
		return (_vertices);
	}

	std::vector<unsigned int> &Mesh::indexes()
	{
		return (_indexes);
	}

	const std::vector<unsigned int> &Mesh::indexes() const
	{
		return (_indexes);
	}
}