#pragma once

#include <vector>
#include <set>
#include <algorithm>
#include <optional>

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	class Mesh
	{
	public:
		using Contract = ContractProvider::Contract;
		using Job = ContractProvider::Job;

		struct VertexIndex
		{
			std::optional<size_t> pointIndex;
			std::optional<size_t> uvIndex;
			std::optional<size_t> normalIndex;

			VertexIndex();
			VertexIndex(size_t p_pointIndex, size_t p_uvIndex);
			VertexIndex(size_t p_pointIndex, size_t p_uvIndex, size_t p_normalIndex);

			bool operator < (const VertexIndex& p_other) const;
			bool operator == (const VertexIndex& p_other) const;
			bool operator != (const VertexIndex& p_other) const;
		};

	private:
		struct Vertex
		{
			spk::Vector3 point;
			spk::Vector2 uv;
			spk::Vector3 normal;
		};

		std::vector<spk::Vector3> _points;
		std::vector<spk::Vector2> _UVs;
		std::vector<spk::Vector3> _normals;

		std::vector<VertexIndex> _vertexIndexes;

		std::vector<Vertex> _vertices;
		std::vector<unsigned int> _indexes;

		spk::ContractProvider _onValidationContractProvider;

		size_t addVertex(VertexIndex p_vertex);

	public:
		Mesh();

		size_t addPoint(const spk::Vector3& p_point);
		size_t addUV(const spk::Vector2& p_uv);
		size_t addNormal(const spk::Vector3& p_normal);

		void addFace(VertexIndex p_vertexA, VertexIndex p_vertexB, VertexIndex p_vertexC);
		void addFace(VertexIndex p_vertexA, VertexIndex p_vertexB, VertexIndex p_vertexC, VertexIndex p_vertexD);

		void validate();
		Contract subscribeToValidation(const Job& p_job);

		const std::vector<spk::Vector3>& points() const;
		const std::vector<spk::Vector2>& UVs() const;
		const std::vector<spk::Vector3>& normals() const;

		std::vector<Vertex>& vertices();
		const std::vector<Vertex>& vertices() const;

		std::vector<unsigned int>& indexes();
		const std::vector<unsigned int>& indexes() const;
	};
}