#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <limits>
#include "design_pattern/spk_notifier.hpp"
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class Mesh
	{
	public:
		struct Data
		{
			struct Vertex
			{
				Vector3 position;
				Vector2 uvs;
				Vector3 normal;
			};

			size_t vertexSize;
			std::vector<float> vertexes;
			std::vector<unsigned int> indexes;
			
		private:
			friend class Mesh;

			void insert(const Vector2& p_data);
			void insert(const Vector3& p_data);
		};

		using Contract = Notifier::Contract;

	public:
		mutable spk::Notifier _editionNotifier;
		std::vector<Vector3> _points;
		std::vector<Vector2> _uvs;
		std::vector<Vector3> _normals;

		std::vector<unsigned int> _indexes;

		struct VertexHash
		{
			std::size_t operator()(const Data::Vertex& vertex) const;
		};

		struct VertexEqual
		{
			bool operator()(const Data::Vertex& lhs, const Data::Vertex& rhs) const;
		};

		Data _bakeWithNormals() const;
		Data _bakeWithoutNormals() const;

	public:
		Mesh();

		Mesh(const Mesh& p_mesh);
		Mesh& operator = (const Mesh& p_mesh);

		Mesh(Mesh&& p_mesh);
		Mesh& operator = (Mesh&& p_mesh);

		std::unique_ptr<Notifier::Contract> subscribeToEdition(const Notifier::Callback& p_callback) const;
		void validate();

		void reservePoints(size_t p_expectedNbPoints);
		void addPoint(const Vector3& p_point);

		void reserveUVs(size_t p_expectedNbUVs);
		void addUVs(const Vector2& p_uvs);

		void reserveNormal(size_t p_expectedNbNormal);
		void addNormal(const Vector3& p_normals);

		void reserveIndex(size_t p_expectedNbIndex);
		void addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex);
		void addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex, unsigned int p_normalIndex);

		void bakeNormals();

		Data bake() const;
	};

	spk::Mesh createSpriteMesh();
}