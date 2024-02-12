#pragma once

#include "design_pattern/spk_tree_node.hpp"
#include "game_engine/spk_transform.hpp"

#include "profiler/spk_time_metric.hpp"

namespace spk
{
	class GameEngine;

	/**
	 * @class GameObject
	 * @brief Represents a game object in the game engine hierarchy.
	 * 
	 * This class represents a fundamental building block in a game engine hierarchy. It combines the features
	 * of an activatable object and a tree node, allowing for the creation of hierarchical structures of game objects.
	 * GameObjects can have child objects, components, and transformations.
	 * 
	 * Usage example:
	 * @code
	 * spk::GameObject parentObject("Parent");
	 * spk::GameObject childObject("Child", &parentObject);
	 * 
	 * // Adding a custom component to a game object
	 * MyCustomComponent* customComponent = parentObject.addComponent<MyCustomComponent>("CustomComponent");
	 * 
	 * // Accessing and manipulating the game object's transformation
	 * spk::Transform& transform = parentObject.transform();
	 * transform.translate({1.0f, 2.0f, 3.0f});
	 * @endcode
	 * 
	 * @see ActivableObject, TreeNode, GameComponent, Transform
	 */
	class GameObject : public spk::ActivateObject, public spk::TreeNode<GameObject>
	{
		friend class GameEngine;

	private:
	#ifndef NDEBUG
		TimeMetric& _timeMetric;
	#endif
		Notifier _onComponentAdditionNotifier;
		std::string _name;
		std::vector<std::string> _tags;
		Transform _transform;
		std::unique_ptr<Transform::Contract> _translationContract;
		std::unique_ptr<Transform::Contract> _scaleContract;
		std::unique_ptr<Transform::RotationContract> _rotationContract;
		std::vector<GameComponent*> _components;

		void render();
		void update();

	public:
		/**
		 * @brief Constructs a GameObject with a specified name.
		 * 
		 * Initializes a new GameObject with the provided name. This object can act as a container for components
		 * and as a node in the hierarchical structure of the scene.
		 * 
		 * @param p_name The name of the GameObject, used for identification within the game engine.
		 */
		GameObject(const std::string& p_name = "Unnamed");

		/**
		 * @brief Constructs a GameObject with a specified name and parent.
		 * 
		 * Initializes a new GameObject with the provided name and sets its parent to the provided GameObject,
		 * automatically adding it to the parent's children list.
		 * 
		 * @param p_name The name of the GameObject.
		 * @param p_parent Pointer to the parent GameObject.
		 */
		GameObject(const std::string& p_name, GameObject* p_parent);

		GameObject(const GameObject& p_other);
		GameObject& operator = (const GameObject& p_other);

		/**
		 * @brief Gets the full name of the GameObject.
		 * 
		 * Constructs and returns the full hierarchical name of the GameObject, incorporating names of parent objects
		 * to generate a unique path-like identifier.
		 * 
		 * @return A string representing the full hierarchical name of the GameObject.
		 */
		std::string fullName() const;

		/**
		 * @brief Gets the name of the GameObject.
		 * 
		 * Returns the name of the GameObject as specified during construction.
		 * 
		 * @return A const reference to the GameObject's name.
		 */
		const std::string& name() const;

		/**
		 * @brief Adds a tag to the GameObject.
		 * 
		 * Tags are simple strings that can be used to categorize or identify game objects within the game world.
		 * This method adds a new tag to this game object, allowing it to be easily found or grouped with others
		 * by the tag.
		 * 
		 * @param p_tag The tag to add to the GameObject.
		 */
		void addTag(const std::string& p_tag);

		/**
		 * @brief Removes a tag from the GameObject.
		 * 
		 * If the game object has been previously tagged with the specified tag, this method removes the tag from
		 * the game object. If the tag is not present, the method has no effect.
		 * 
		 * @param p_tag The tag to remove from the GameObject.
		 */
		void removeTag(const std::string& p_tag);

		/**
		 * @brief Retrieves all tags associated with the GameObject.
		 * 
		 * This method provides access to all the tags that have been added to the game object. Tags can be used to
		 * categorize, identify, or filter game objects within the game world.
		 * 
		 * @return A constant reference to a vector containing all tags added to the GameObject.
		 */
		const std::vector<std::string>& tags() const;

		/**
		 * @brief Accesses the GameObject's transformation.
		 * 
		 * Provides mutable access to the GameObject's Transform component, allowing for manipulation of the
		 * GameObject's position, rotation, and scale in the game world.
		 * 
		 * @return A reference to the GameObject's Transform component.
		 */
		Transform& transform();

		/**
		 * @brief Accesses the GameObject's transformation (const version).
		 * 
		 * Provides immutable access to the GameObject's Transform component.
		 * 
		 * @return A const reference to the GameObject's Transform component.
		 */
		const Transform& transform() const;

		/**
		 * @brief Computes the global position of the GameObject.
		 * 
		 * Calculates and returns the GameObject's global position, taking into account the positions of parent objects
		 * in the scene hierarchy.
		 * 
		 * @return A Vector3 representing the global position of the GameObject.
		 */
		spk::Vector3 globalPosition() const;

		/**
		 * @brief Computes the global scale of the GameObject.
		 * 
		 * Calculates and returns the GameObject's global scale, taking into account the scales of parent objects
		 * in the scene hierarchy.
		 * 
		 * @return A Vector3 representing the global scale of the GameObject.
		 */
		spk::Vector3 globalScale() const;

		/**
		 * @brief Computes the global rotation of the GameObject.
		 * 
		 * Calculates and returns the GameObject's global rotation, taking into account the rotations of parent objects
		 * in the scene hierarchy.
		 * 
		 * @return A Vector3 representing the global rotation of the GameObject.
		 */
		spk::Quaternion globalRotation() const;

		/**
		 * @brief Adds a component of a specified type to the GameObject.
		 * 
		 * Dynamically creates a new component of the specified type, attaches it to the GameObject, and returns
		 * a pointer to the newly created component. This method allows for the addition of custom behavior
		 * and functionality to the GameObject through components.
		 * 
		 * @tparam TComponentName The type of the component to add.
		 * @tparam Args The types of the arguments to pass to the component's constructor.
		 * @param p_args The arguments to pass to the component's constructor.
		 * @return A pointer to the newly created component.
		 */
		template<typename TComponentName, typename ... Args>
		TComponentName* addComponent(Args&& ... p_args)
		{
			GameComponent::_creatingObject = this;

			TComponentName* result = new TComponentName(std::forward<Args>(p_args)...);
			_components.push_back(result);

			GameComponent::_creatingObject = nullptr;

			_onComponentAdditionNotifier.notify_all();

			return (result);
		}

		/**
		 * @brief Retrieves the first component of a specified type and name from the GameObject.
		 * 
		 * Searches the GameObject's components for the first component that matches the specified type and,
		 * optionally, the specified name. Returns a pointer to the component if found, or nullptr otherwise.
		 * 
		 * @tparam TComponentName The type of the component to search for.
		 * @param p_expectedName The name of the component to search for (optional).
		 * @return A pointer to the component if found, or nullptr otherwise.
		 */
		template<typename TComponentName>
		TComponentName* getComponent(const std::string& p_expectedName = "")
		{
			for (auto& component : _components)
			{
				TComponentName* castedPointer = dynamic_cast<TComponentName*>(component);
				if (castedPointer != nullptr && (p_expectedName == "" || castedPointer->name() == p_expectedName))
				{
					return (castedPointer);
				}
			}
			return (nullptr);
		}

		/**
		 * @brief Retrieves the first component of a specified type and name from the GameObject, as a const pointer.
		 * 
		 * Searches the GameObject's components for the first component that matches the specified type and,
		 * optionally, the specified name. Returns a pointer to the component if found, or nullptr otherwise.
		 * 
		 * @tparam TComponentName The type of the component to search for.
		 * @param p_expectedName The name of the component to search for (optional).
		 * @return A const pointer to the component if found, or nullptr otherwise.
		 */
		template<typename TComponentName>
		const TComponentName* getComponent(const std::string& p_expectedName = "") const
		{
			for (auto& component : _components)
			{
				TComponentName* castedPointer = dynamic_cast<TComponentName*>(component);
				if (castedPointer != nullptr && (p_expectedName == "" || castedPointer->name() == p_expectedName))
				{
					return (castedPointer);
				}
			}
			return (nullptr);
		}

		/**
		 * @brief Retrieves all components of a specified type from the GameObject.
		 * 
		 * Searches the GameObject's components for all components that match the specified type and returns
		 * a vector of pointers to these components.
		 * 
		 * @tparam TComponentName The type of the components to search for.
		 * @return A vector of pointers to the components of the specified type.
		 */
		template<typename TComponentName>
		std::vector<TComponentName*> getComponentList()
		{
			std::vector<TComponentName*> result;

			for (auto& component : _components)
			{
				TComponentName* castedPointer = dynamic_cast<TComponentName*>(component);
				if (castedPointer != nullptr)
				{
					result.push_back(castedPointer);
				}
			}
			return (result);
		}

		/**
		 * @brief Retrieves all components of a specified type from the GameObject, as const pointers.
		 * 
		 * Searches the GameObject's components for all components that match the specified type and returns
		 * a vector of pointers to these components.
		 * 
		 * @tparam TComponentName The type of the components to search for.
		 * @return A vector of const pointers to the components of the specified type.
		 */
		template<typename TComponentName>
		std::vector<const TComponentName*> getComponentList() const
		{
			std::vector<TComponentName*> result;

			for (auto& component : _components)
			{
				TComponentName* castedPointer = dynamic_cast<TComponentName*>(component);
				if (castedPointer != nullptr)
				{
					result.push_back(castedPointer);
				}
			}
			return (result);
		}
	};
}