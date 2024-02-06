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
	/**
	 * @brief Represents a mesh for defining 3D geometry in the game.
	 *
	 * This class is used to create and manage 3D meshes, which are essential for defining the geometry of objects in the game.
	 * It provides functions for adding vertices, UVs, normals, and indices to the mesh and allows for baking the final mesh data.
	 * 
	 * In addition, Mesh are automaticaly optimize when baked to reduce memory allocation on GPU.
	 *
	 * Usage example:
	 * @code
	 * // Create a mesh instance
	 * spk::Mesh mesh;
	 *
	 * // Reserve space for points, UVs, normals, and indices
	 * mesh.reservePoints(4);
	 * mesh.reserveUVs(4);
	 * mesh.reserveNormal(1);
	 * mesh.reserveIndex(6);
	 *
	 * result.addPoint(spk::Vector3(0.5f, 0.0f, 0.5f));
	 * result.addPoint(spk::Vector3(-0.5f, 0.0f, 0.5f));
	 * result.addPoint(spk::Vector3(0.5f, 0.0f, -0.5f));
	 * result.addPoint(spk::Vector3(-0.5f, 0.0f, -0.5f));
	 * 
	 * result.addUVs(spk::Vector2(1, 0));
	 * result.addUVs(spk::Vector2(0, 0));
	 * result.addUVs(spk::Vector2(1, 1));
	 * result.addUVs(spk::Vector2(0, 1));
	 * 
	 * result.addNormal(spk::Vector3(0, 1, 0));

	 * result.addIndex(0, 0, 0);
	 * result.addIndex(1, 1, 0);
	 * result.addIndex(2, 2, 0);
	 * result.addIndex(2, 2, 0);
	 * result.addIndex(1, 1, 0);
	 * result.addIndex(3, 3, 0);
	 *
	 * // Bake the mesh to obtain the final mesh data
	 * spk::Mesh::Data finalMeshData = mesh.bake();
	 * @endcode
	 *
	 * @see IVector2, IVector3
	 */

	class Mesh
	{
	public:

		/**
		 * @brief Represents the data structure for a baked mesh.
		 * 
		 * This struct holds the final mesh data after baking, including vertex information, vertex size, vertexes, and indexes.
		 * It is used to obtain the mesh data once it has been prepared for rendering.
		 * 
		 * Usage example:
		 * @code
		 * // Create a mesh instance
		 * spk::Mesh mesh;
		 * 
		 * // Reserve space for points, UVs, normals, and indices, and add vertices, UVs, normals, and indices to the mesh
		 * // ...
		 * 
		 * // Bake the mesh to obtain the final mesh data
		 * spk::Mesh::Data finalMeshData = mesh.bake();
		 * 
		 * // Access and use the mesh data
		 * size_t vertexSize = finalMeshData.vertexSize;
		 * const std::vector<float>& vertexes = finalMeshData.vertexes;
		 * const std::vector<unsigned int>& indexes = finalMeshData.indexes;
		 * @endcode
		 * 
		 * @see Mesh
		 */
		struct Data
		{
			size_t vertexSize;
			std::vector<float> vertexes;
			std::vector<unsigned int> indexes;

		private:
			friend class Mesh;
			friend class VertexHash;
			friend class VertexEqual;
			
			struct Vertex
			{
				Vector3 position;
				Vector2 uvs;
				Vector3 normal;
			};

			void insert(const Vector2 &p_data);
			void insert(const Vector3 &p_data);
		};

		using Contract = Notifier::Contract;

	private:
		struct VertexHash
		{
			std::size_t operator()(const Data::Vertex &vertex) const;
		};

		struct VertexEqual
		{
			bool operator()(const Data::Vertex &lhs, const Data::Vertex &rhs) const;
		};
		
	public:
		mutable spk::Notifier _editionNotifier;
		std::vector<Vector3> _points;
		std::vector<Vector2> _uvs;
		std::vector<Vector3> _normals;

		std::vector<unsigned int> _indexes;

		Data _bakeWithNormals() const;
		Data _bakeWithoutNormals() const;

	public:
		Mesh();

		Mesh(const Mesh &p_mesh);
		Mesh &operator=(const Mesh &p_mesh);

		Mesh(Mesh &&p_mesh);
		Mesh &operator=(Mesh &&p_mesh);

		std::unique_ptr<Notifier::Contract> subscribeToEdition(const Notifier::Callback &p_callback) const;
		void validate();

		void reservePoints(size_t p_expectedNbPoints);
		void addPoint(const Vector3 &p_point);

		void reserveUVs(size_t p_expectedNbUVs);
		void addUVs(const Vector2 &p_uvs);

		void reserveNormal(size_t p_expectedNbNormal);
		void addNormal(const Vector3 &p_normals);

		void reserveIndex(size_t p_expectedNbIndex);
		void addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex);
		void addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex, unsigned int p_normalIndex);

		void bakeNormals();

		Data bake() const;
	};

	spk::Mesh createSpriteMesh();
}