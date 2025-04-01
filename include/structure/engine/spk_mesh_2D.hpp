#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

#include "structure/math/spk_vector2.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	class Mesh2D
	{
	public:
		using Contract = ContractProvider::Contract;
		using Job = ContractProvider::Job;

		struct VertexIndex
		{
			std::optional<size_t> pointIndex;
			std::optional<size_t> uvIndex;

			VertexIndex();
			VertexIndex(size_t p_pointIndex, size_t p_uvIndex);

			bool operator<(const VertexIndex &p_other) const;
			bool operator==(const VertexIndex &p_other) const;
			bool operator!=(const VertexIndex &p_other) const;
		};

	private:
		struct Vertex
		{
			spk::Vector2 point;
			spk::Vector2 uv;
		};

		std::vector<spk::Vector2> _points;
		std::vector<spk::Vector2> _UVs;

		std::vector<VertexIndex> _vertexIndexes;

		std::vector<unsigned int> _indexes;

		std::vector<Vertex> _vertices;

		spk::ContractProvider _onValidationContractProvider;

		size_t addVertex(const VertexIndex &p_vertex);

	public:
		Mesh2D();

		size_t addPoint(const spk::Vector2 &p_point);

		size_t addUV(const spk::Vector2 &p_uv);

		void addFace(const VertexIndex &p_vertexA, const VertexIndex &p_vertexB, const VertexIndex &p_vertexC);

		void addFace(const VertexIndex &p_vertexA, const VertexIndex &p_vertexB, const VertexIndex &p_vertexC, const VertexIndex &p_vertexD);

		void validate();
		Contract subscribeToValidation(const Job &p_job);

		const std::vector<spk::Vector2> &points() const;
		const std::vector<spk::Vector2> &UVs() const;

		std::vector<Vertex> &vertices();
		const std::vector<Vertex> &vertices() const;

		std::vector<unsigned int> &indexes();
		const std::vector<unsigned int> &indexes() const;
	};
}