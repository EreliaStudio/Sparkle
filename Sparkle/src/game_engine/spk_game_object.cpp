#include "game_engine/spk_game_object.hpp"

namespace spk
{
	void GameObject::render()
	{
		if (isActive() == true)
			return ;
				
		for (auto& component : _components)
		{
			component->_onRender();
		}

		for (auto& child : children())
		{
			child->render();
		}
	}

	void GameObject::update()
	{
		if (isActive() == true)
			return ;

		for (auto& component : _components)
		{
			component->_onUpdate();
		}

		for (auto& child : children())
		{
			child->update();
		}
	}

	GameObject::GameObject(const std::string& p_name) :
		_name(p_name),
		_transform(),
		_translationContract(_transform.translation.subscribe([&](){
			for (auto& child : children())
			{
				child->transform().translation.notify_all();
			}
		})),
		_scaleContract(_transform.scale.subscribe([&](){
			for (auto& child : children())
			{
				child->transform().scale.notify_all();
			}
		})),
		_rotationContract(_transform.rotation.subscribe([&](){
			for (auto& child : children())
			{
				child->transform().rotation.notify_all();
			}
		}))
	{
		_transform._bind(this);
	}

	GameObject::GameObject(const std::string& p_name, GameObject* p_parent) :
		GameObject(p_name)
	{
		p_parent->addChild(this);
	}

	std::string GameObject::fullName() const
	{
		if (parent() == nullptr)
			return (name());
		return (parent()->fullName() + "::" + name());
	}
	
	const std::string& GameObject::name() const
	{
		return (_name);
	}

	Transform& GameObject::transform()
	{
		return (_transform);
	}

	const Transform& GameObject::transform() const
	{
		return (_transform);
	}

	spk::Vector3 GameObject::globalPosition() const
	{
		spk::Vector3 result = 0;
	
		result += _transform.translation.get();

		if (parent() != nullptr)
		{
			result *= parent()->globalScale();
			result = parent()->globalRotation().applyRotation(result);
			result += parent()->globalPosition();
		}

		return (result);
	}

	spk::Vector3 GameObject::globalScale() const
	{
		if (parent() == nullptr)
			return (_transform.scale.get());

		return (parent()->globalScale() * _transform.scale.get());
	}

	spk::Quaternion GameObject::globalRotation() const
	{
		if (parent() == nullptr)
			return (_transform.rotation.get());

		return (parent()->globalRotation() * _transform.rotation.get());
	}
}