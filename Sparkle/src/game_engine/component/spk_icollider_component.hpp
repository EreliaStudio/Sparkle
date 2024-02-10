#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "game_engine/spk_mesh.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	/**
     * @class ICollider
     * @brief Abstract base class for collider components within the game engine.
     * 
     * ICollider provides a common interface for collider components, facilitating collision detection
     * between game objects. It is designed to be extended by specific collider implementations such
     * as box colliders, sphere colliders, or mesh colliders. This class integrates closely with the
     * GameObject's transformation properties and Mesh data to calculate collisions accurately.
     *
     * Usage example:
     * @code
     * spk::GameObject playerObject("Player");
     * auto playerMesh = new spk::Mesh("PlayerMesh");
     * 
     * // Add a specific collider type to the game object, here assuming a BoxCollider is a derived class of ICollider
     * auto collider = playerObject.addComponent<spk::BoxCollider>("PlayerCollider");
     * collider->setMesh(playerMesh);
     * 
     * // Collision check with another collider
     * spk::GameObject enemyObject("Enemy");
     * auto enemyCollider = enemyObject.addComponent<spk::BoxCollider>("EnemyCollider");
     * enemyCollider->setMesh(enemyMesh); // Assume enemyMesh is defined elsewhere
     * 
     * if (collider->isIntersecting(enemyCollider)) {
     *     std::cout << "Collision detected between Player and Enemy!" << std::endl;
     * }
     * @endcode
     *
     * Note: This class requires specific collider types to implement the isIntersecting method
     * to determine collisions with other colliders.
     * 
     * @see GameComponent, Mesh
     */
	class ICollider : public spk::GameComponent
	{
	private:
		const Mesh* _mesh;

		std::unique_ptr<Notifier::Contract> _translationContract;
		std::unique_ptr<Notifier::Contract> _scaleContract;
		std::unique_ptr<Notifier::Contract> _rotationContract;

		void _onRender()
		{

		}

		void _onUpdate()
		{

		}

		virtual void _onMeshEdition() = 0;

	public:
		/**
         * @brief Constructs an ICollider component with a specified name.
         * 
         * Initializes a collider component, setting up subscriptions to transformation changes
         * of its owner GameObject. These subscriptions ensure the collider updates its state
         * in response to the GameObject's movement, scaling, and rotation.
         * 
         * @param p_name The name of the collider component.
         */
		ICollider(const std::string& p_name) :
			GameComponent(p_name),
			_mesh(nullptr),
			_translationContract(owner()->transform().translation.subscribe([&](){_onMeshEdition();})),
			_scaleContract(owner()->transform().scale.subscribe([&](){_onMeshEdition();})),
			_rotationContract(owner()->transform().rotation.subscribe([&](){_onMeshEdition();}))
		{
			
		}

		/**
         * @brief Determines if the collider intersects with another collider.
         * 
         * Pure virtual function that must be implemented by derived collider classes to check for
         * intersections with another collider. This method is used for collision detection between
         * game objects.
         * 
         * @param p_otherCollider Pointer to another collider component to check for intersection.
         * @return bool True if there is an intersection, false otherwise.
         */
		virtual bool isIntersecting(const ICollider* p_otherCollider) = 0;

		/**
         * @brief Sets the mesh used for collision detection.
         * 
         * Assigns a mesh to the collider. The mesh is used to calculate the collider's bounding
         * volume for collision detection purposes.
         * 
         * @param p_mesh Pointer to the Mesh object.
         */
		void setMesh(const Mesh* p_mesh)
		{
			_mesh = p_mesh;
			_onMeshEdition();
		}
		
		/**
         * @brief Gets the mesh associated with the collider.
         * 
         * Returns a pointer to the mesh currently used by the collider for collision detection.
         * 
         * @return const spk::Mesh* Pointer to the Mesh object.
         */
		const spk::Mesh* mesh() const
		{
			return (_mesh);
		}
	};
}