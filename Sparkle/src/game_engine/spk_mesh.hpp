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
	 * @class Mesh
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
		 * @class Data
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
			size_t vertexSize; //!< The size of one single element contained in this Data.
			std::vector<float> vertexes; //!< The list of data, as row float, contained in this Data.
			std::vector<unsigned int> indexes; //!< The list of indexes as unsigned int, contained in this Data.

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

		/**
		 * @brief Describe what kind of contract is provided by the mesh uppon subscription to it's edition.
		*/
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
		
	private:
		mutable spk::Notifier _editionNotifier;
		std::vector<Vector3> _points;
		std::vector<Vector2> _uvs;
		std::vector<Vector3> _normals;

		std::vector<unsigned int> _indexes;

		Data _bakeWithNormals() const;
		Data _bakeWithoutNormals() const;

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new Mesh instance. This instance is ready to have vertices, UVs, normals, and indices added to it
		 * in preparation for creating 3D geometry.
		 */
		Mesh();

		/**
		 * @brief Copy constructor.
		 * 
		 * Creates a new Mesh instance as a copy of an existing mesh. All vertices, UVs, normals, and indices from the
		 * original mesh are copied to the new instance.
		 * 
		 * @param p_mesh The Mesh instance to copy from.
		 */
		Mesh(const Mesh &p_mesh);

		/**
		 * @brief Copy assignment operator.
		 * 
		 * Copies all mesh data from another Mesh instance to this one. This includes vertices, UVs, normals, and indices.
		 * 
		 * @param p_mesh The Mesh instance to copy from.
		 * @return A reference to this Mesh instance after copying the data.
		 */
		Mesh &operator=(const Mesh &p_mesh);

		/**
		 * @brief Move constructor.
		 * 
		 * Transfers the mesh data from another Mesh instance to this one, leaving the original instance empty. This is more
		 * efficient than copying for temporary Mesh instances.
		 * 
		 * @param p_mesh The Mesh instance to move from.
		 */
		Mesh(Mesh &&p_mesh);

		/**
		 * @brief Move assignment operator.
		 * 
		 * Moves all mesh data from another Mesh instance to this one, efficiently transferring ownership of the data and
		 * leaving the original instance empty.
		 * 
		 * @param p_mesh The Mesh instance to move from.
		 * @return A reference to this Mesh instance after moving the data.
		 */
		Mesh &operator=(Mesh &&p_mesh);

		/**
		 * @brief Subscribes a callback to be notified of mesh edition events.
		 * 
		 * Allows external entities to subscribe to notifications about changes to the mesh. This can be used to react to
		 * modifications in real-time, such as updating a rendered object when its mesh changes.
		 * 
		 * @param p_callback The callback function to be invoked on mesh edition events.
		 * @return A unique_ptr to a Notifier::Contract, managing the lifecycle of the subscription.
		 */
		std::unique_ptr<Notifier::Contract> subscribeToEdition(const Notifier::Callback &p_callback) const;

		/**
		 * @brief Marks the mesh as validated, indicating it is ready for use or rendering.
		 * 
		 * This method should be called after all modifications to the mesh have been completed and the mesh is ready to be
		 * used in the game. It triggers any subscribed edition notifications.
		 */
		void validate();

		/**
		 * @brief Reserves space for a specified number of points in the mesh.
		 * 
		 * Preallocates memory for a given number of points (vertices) to optimize memory usage and performance when adding
		 * new points to the mesh.
		 * 
		 * @param p_expectedNbPoints The number of points to reserve space for.
		 */
		void reservePoints(size_t p_expectedNbPoints);

		/**
		 * @brief Adds a new point (vertex) to the mesh.
		 * 
		 * Inserts a new 3D point into the mesh's list of points. This point can then be referenced by indices when defining
		 * geometry.
		 * 
		 * @param p_point The 3D position of the point to add.
		 */
		void addPoint(const Vector3 &p_point);

		const std::vector<spk::Vector3>& points() const;

		/**
		 * @brief Reserves space for a specified number of UV coordinates in the mesh.
		 * 
		 * Preallocates memory for a given number of UV coordinate pairs to optimize memory usage and performance when adding
		 * new UVs to the mesh.
		 * 
		 * @param p_expectedNbUVs The number of UV coordinate pairs to reserve space for.
		 */
		void reserveUVs(size_t p_expectedNbUVs);

		/**
		 * @brief Adds a new set of UV coordinates to the mesh.
		 * 
		 * Inserts a new pair of UV coordinates into the mesh's list of UVs. These coordinates are used to map textures to the
		 * mesh's geometry.
		 * 
		 * @param p_uvs The UV coordinates to add.
		 */
		void addUVs(const Vector2 &p_uvs);

		const std::vector<spk::Vector2>& UVs() const;

		/**
		 * @brief Reserves space for a specified number of normals in the mesh.
		 * 
		 * Preallocates memory for a given number of normals to optimize memory usage and performance when adding new normals
		 * to the mesh.
		 * 
		 * @param p_expectedNbNormal The number of normals to reserve space for.
		 */
		void reserveNormal(size_t p_expectedNbNormal);

		/**
		 * @brief Adds a new normal to the mesh.
		 * 
		 * Inserts a new normal vector into the mesh's list of normals. Normals are used to determine how light interacts
		 * with the surface of the mesh.
		 * 
		 * @param p_normals The normal vector to add.
		 */
		void addNormal(const Vector3 &p_normals);

		const std::vector<spk::Vector3>& normals() const;

		/**
		 * @brief Reserves space for a specified number of indices in the mesh.
		 * 
		 * Preallocates memory for a given number of indices to optimize memory usage and performance when adding new indices
		 * to the mesh.
		 * 
		 * @param p_expectedNbIndex The number of indices to reserve space for.
		 */
		void reserveIndex(size_t p_expectedNbIndex);

		/**
		 * @brief Adds a new index to the mesh.
		 * 
		 * Inserts a new index into the mesh's list of indices. Indices are used to define the order in which points are
		 * connected to form the mesh's geometry.
		 * 
		 * @param p_pointIndex The index of the point.
		 * @param p_uvsIndex The index of the UV coordinate.
		 * @param p_normalIndex The index of the normal (optional).
		 */
		void addIndex(unsigned int p_pointIndex, unsigned int p_uvsIndex, unsigned int p_normalIndex = std::numeric_limits<unsigned int>::max());

		const std::vector<unsigned int>& indexes() const;

		/**
		 * @brief Bakes the mesh data, optimizing it for rendering.
		 * 
		 * Processes the added points, UVs, normals, and indices to create a final, optimized mesh data structure suitable for
		 * rendering. This includes deduplicating vertices and preparing the data for GPU upload.
		 * 
		 * @return The Data struct containing the baked mesh data.
		 */
		Data bake() const;

		/**
		 * @brief Generates normals for the mesh based on the vertices and indices already added.
		 * 
		 * This method calculates normals for each vertex in the mesh, essential for proper lighting and shading effects in 3D graphics.
		 * Normals are computed to be perpendicular to the surface defined by the vertices and indices of the mesh, facilitating
		 * the realistic rendering of light interactions.
		 * 
		 * @code
		 * spk::Mesh myMesh = spk::Mesh();
		 * 
		 * // Complete the mesh by inserting vertices and indexes.
		 * 
		 * //Calling the bakeNormals method.
		 * myMesh.bakeNormals();
		 * 
		 * // The mesh now contain one normal for each vertices stored, and for each triangles containing the vertices.
		 * @endcode
		 */
		void bakeNormals();
	};

	/**
	 * @brief Creates a default sprite mesh suitable for 2D and 3D sprite rendering.
	 * 
	 * This static method generates a predefined mesh optimized for rendering 2D and 3D sprites. The generated mesh includes
	 * vertices and texture coordinates configured for a standard quad, making it ideal for use with sprite sheets
	 * and 2D and 3D graphics rendering.
	 * 
	 * @return A Mesh instance containing the geometry for a 2D and 3D sprite, including vertices arranged in a quad and corresponding
	 * UV coordinates for texture mapping.
	 * 
	 * Usage example:
	 * @code
	 * spk::Mesh spriteMesh = spk::createSpriteMesh();
	 * // The spriteMesh can now be used with a Texture and Shader to render 2D and 3D sprites.
	 * @endcode
	 * 
	 * @note The generated sprite mesh is typically used in conjunction with orthographic projection in 2D and 3D games or UI elements,
	 * where the depth (z-axis) is not a factor in rendering. Its indexes are (0, 1, 2, 2, 1, 3).
	 */
	spk::Mesh createSpriteMesh();
}