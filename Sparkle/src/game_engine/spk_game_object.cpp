#include "game_engine/spk_game_object.hpp"

namespace spk
{
	void GameObject::render()
	{
		if (isActive() == true)
			return ;
				
		for (auto& component : _components)
		{
			component->onRender();
		}

		for (auto& child : childrens())
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
			component->onUpdate();
		}

		for (auto& child : childrens())
		{
			child->update();
		}
	}

	GameObject::GameObject(const std::string& p_name) :
		_name(p_name),
		_transform(),
		_translationContract(_transform.translation.subscribe([&](){
			for (auto& child : childrens())
			{
				child->transform().translation.notify_all();
			}
		})),
		_sclaeContract(_transform.scale.subscribe([&](){
			for (auto& child : childrens())
			{
				child->transform().scale.notify_all();
			}
		})),
		_rotationContract(_transform.rotation.subscribe([&](){
			for (auto& child : childrens())
			{
				child->transform().rotation.notify_all();
			}
		}))
	{

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
		if (parent() == nullptr)
			return (_transform.translation.get());

		spk::Vector3 result;

		result = _transform.translation.get().rotate(parent()->transform().rotation) * parent()->transform().scale;

		return (result + parent()->globalPosition());
	}

	spk::Vector3 GameObject::globalScale() const
	{
		if (parent() == nullptr)
			return (_transform.scale.get());

		return (parent()->globalScale() * _transform.scale.get());
	}

	spk::Vector3 GameObject::globalRotation() const
	{
		if (parent() == nullptr)
			return (_transform.rotation.get());

		return (parent()->globalRotation() + _transform.rotation.get());
	}
}