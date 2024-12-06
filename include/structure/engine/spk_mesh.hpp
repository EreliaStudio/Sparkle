#pragma once

#include <vector>
#include <set>
#include <algorithm>
#include <optional>

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	class Mesh
	{
	private:
		struct Vertex
		{
			spk::Vector3 point;
			spk::Vector2 uv;
			spk::Vector3 normal;
		};

		struct VertexIndex
		{
			std::optional<size_t> pointIndex;
			std::optional<size_t> uvIndex;
			std::optional<size_t> normalIndex;

			VertexIndex() :
				pointIndex(),
				uvIndex(),
				normalIndex()
			{

			}	

			VertexIndex(size_t p_pointIndex, size_t p_uvIndex) :
				pointIndex(p_pointIndex),
				uvIndex(p_uvIndex),
				normalIndex()
			{

			}

			VertexIndex(size_t p_pointIndex, size_t p_uvIndex, size_t p_normalIndex) :
				pointIndex(p_pointIndex),
				uvIndex(p_uvIndex),
				normalIndex(p_normalIndex)
			{

			}
		};

		std::vector<spk::Vector3> _points;
		std::vector<spk::Vector2> _UVs;
		std::vector<spk::Vector3> _normals;

		std::vector<VertexIndex> _vertexIndexes;

		bool _baked;
		std::vector<Vertex> _vertices;
		std::vector<unsigned int> _indexes;

		size_t addVertex(VertexIndex p_vertex)
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

	public:
		Mesh()
		{

		}

		size_t addPoint(const spk::Vector3& p_point)
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

		size_t addUV(const spk::Vector2& p_uv)
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

		size_t addNormal(const spk::Vector3& p_normal)
		{
			auto it = std::find(_normals.begin(), _normals.end(), p_normal);
			if (it != _normals.end())
			{
				return std::distance(_normals.begin(), it);
			}
			else
			{
				_normals.push_back(p_normal);
				return _normals.size() - 1;
			}
		}

		void addFace(VertexIndex p_vertexA, VertexIndex p_vertexB, VertexIndex p_vertexC)
		{
			if (p_vertexA.pointIndex.has_value() == false || _points.size() <= p_vertexA.pointIndex.value())
			{
				throw std::runtime_error("Error loading a face with invalid index [" + (p_vertexA.pointIndex.has_value() == true ? std::to_string(p_vertexA.pointIndex.value()) : "No value") + "] over [" + std::to_string(_points.size()) + "] points");	
			}
			if (p_vertexB.pointIndex.has_value() == false || _points.size() <= p_vertexB.pointIndex.value())
			{
				throw std::runtime_error("Error loading a face with invalid index [" + (p_vertexB.pointIndex.has_value() == true ? std::to_string(p_vertexB.pointIndex.value()) : "No value") + "] over [" + std::to_string(_points.size()) + "] points");	
			}
			if (p_vertexC.pointIndex.has_value() == false || _points.size() <= p_vertexC.pointIndex.value())
			{
				throw std::runtime_error("Error loading a face with invalid index [" + (p_vertexC.pointIndex.has_value() == true ? std::to_string(p_vertexC.pointIndex.value()) : "No value") + "] over [" + std::to_string(_points.size()) + "] points");	
			}

			if (p_vertexA.normalIndex.has_value() == false ||
				p_vertexB.normalIndex.has_value() == false ||
				p_vertexC.normalIndex.has_value() == false)
			{

				spk::Vector3 pointA = _points[p_vertexA.pointIndex.value()];
				spk::Vector3 pointB = _points[p_vertexB.pointIndex.value()];
				spk::Vector3 pointC = _points[p_vertexC.pointIndex.value()];

				spk::Vector3 newNormal = (pointB - pointA).cross(pointC - pointA);

				size_t value = addNormal(newNormal);
			
				if (p_vertexA.normalIndex.has_value() == false)
				{
					p_vertexA.normalIndex = value;
				}
			
				if (p_vertexB.normalIndex.has_value() == false)
				{
					p_vertexB.normalIndex = value;
				}
			
				if (p_vertexB.normalIndex.has_value() == false)
				{
					p_vertexB.normalIndex = value;
				}
			}

			_indexes.push_back(addVertex(p_vertexA));
			_indexes.push_back(addVertex(p_vertexB));
			_indexes.push_back(addVertex(p_vertexC));

			_baked = false;
		}

		void addFace(VertexIndex p_vertexA, VertexIndex p_vertexB, VertexIndex p_vertexC, VertexIndex p_vertexD)
		{
			addFace(p_vertexA, p_vertexB, p_vertexC);
			addFace(p_vertexC, p_vertexB, p_vertexD);
		}

		void bake()
		{
			_vertices.clear();

			for (const auto& vertexIndex : _vertexIndexes)
			{
				if (vertexIndex.pointIndex.has_value() == false || 
					vertexIndex.uvIndex.has_value() == false || 
					vertexIndex.normalIndex.has_value() == false)
				{
					throw std::runtime_error("Error: VertexIndex contains unset optional values. Ensure all indices are set before baking.");
				}
			
				_vertices.push_back({
					.point = _points[vertexIndex.pointIndex.value()],
					.uv = _UVs[vertexIndex.uvIndex.value()],
					.normal = _normals[vertexIndex.normalIndex.value()]
				});
			}
			
			_baked = true;
		}

		const std::vector<Vertex>& vertices() const
		{
			if (_baked == false)
				throw std::runtime_error("Can't access unbaked mesh vertices. call .bake() before calling .vertices()");
			return (_vertices);
		}		

		const std::vector<Vertex>& vertices() const
		{
			return (_vertices);
		}		
	};
}