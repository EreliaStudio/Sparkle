#pragma once

#include "design_pattern/spk_tree_node.hpp"
#include "game_engine/spk_transform.hpp"

namespace spk
{
	class GameObject : public spk::ActivateObject, public spk::TreeNode<GameObject>
	{
		friend class GameEngine;

	private:
		std::string _name;
		Transform _transform;
		std::unique_ptr<Transform::Contract> _translationContract;
		std::unique_ptr<Transform::Contract> _sclaeContract;
		std::unique_ptr<Transform::Contract> _rotationContract;
		std::vector<GameComponent*> _components;

		void render();
		void update();

	public:
		GameObject(const std::string& p_name);
		GameObject(const std::string& p_name, GameObject* p_parent);

		std::string fullName() const;
		const std::string& name() const;

		Transform& transform();
		const Transform& transform() const;

		spk::Vector3 globalPosition() const;
		spk::Vector3 globalScale() const;
		spk::Vector3 globalRotation() const;

		template<typename TComponentName, typename ... Args>
		TComponentName* addComponent(Args&& ... p_args)
		{
			GameComponent::_creatingObject = this;

			TComponentName* result = new TComponentName(std::forward<Args>(p_args)...);
			_components.push_back(result);

			GameComponent::_creatingObject = nullptr;

			return (result);
		}

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
	};
}