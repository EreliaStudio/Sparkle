#pragma once

#include "game_engine/component/spk_icollider_component.hpp"
#include "game_engine/spk_mesh.hpp"

namespace spk
{
	/**
     * @class MeshCollider2D
     * @brief A 2D collider component for sprites, providing collision detection based on axis-aligned bounding boxes (AABB).
     *
     * MeshCollider2D extends the ICollider interface to implement collision detection for 2D sprites. It calculates
     * an axis-aligned bounding box (AABB) for the sprite based on its mesh and transformation properties, allowing for
     * efficient collision detection with other colliders in the game world. This component is suitable for games where
     * simple 2D collision detection is required.
     *
     * Usage example:
     * @code
     * spk::GameObject player("Player");
     * auto playerMesh = std::make_shared<spk::Mesh>("playerMeshPath");
     * auto playerCollider = player.addComponent<spk::MeshCollider2D>("PlayerCollider");
     * playerCollider->setMesh(playerMesh.get());
     * 
     * spk::GameObject enemy("Enemy");
     * auto enemyMesh = std::make_shared<spk::Mesh>("enemyMeshPath");
     * auto enemyCollider = enemy.addComponent<spk::MeshCollider2D>("EnemyCollider");
     * enemyCollider->setMesh(enemyMesh.get());
     * 
     * if (playerCollider->isIntersecting(enemyCollider)) {
     *     std::cout << "Player has collided with Enemy!" << std::endl;
     * }
     * @endcode
     */
	class MeshCollider2D : public ICollider
	{
	private:
		spk::Vector3 _min;
		spk::Vector3 _max;

		spk::Vector3 _computeMin(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh);
		spk::Vector3 _computeMax(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh);

		void _onMeshEdition();

	public:
          /**
         * @brief Constructs a MeshCollider2D with a specified name.
         * 
         * Initializes the collider with a name, setting up its internal structures for collision detection. The
         * collider starts without an associated mesh, which must be set using setMesh for the collider to function.
         * 
         * @param p_name The name of the collider component.
         */
		MeshCollider2D(const std::string& p_name);

          /**
         * @brief Checks if this collider intersects with another collider.
         * 
         * Determines if the axis-aligned bounding box (AABB) of this sprite collider intersects with the AABB of
         * another collider. This method is used to detect collisions between this sprite and other objects in the game world.
         * 
         * @param p_otherCollider Pointer to another collider component to check for intersection.
         * @return bool True if there is an intersection, false otherwise.
         */
		bool isIntersecting(const ICollider* p_otherCollider);
	};
}