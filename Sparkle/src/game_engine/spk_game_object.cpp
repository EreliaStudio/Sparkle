#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void GameObject::render()
	{
		if (isActive() == false)
			return ;				
			
		for (auto& component : _components)
		{
			component->render();
		}

		for (auto& child : children())
		{
			child->render();
		}
	}

	void GameObject::update()
	{
		if (isActive() == false)
			return ;

	#ifndef NDEBUG
		_timeMetric.start();
	#endif
		for (auto& component : _components)
		{
			component->update();
		}
	#ifndef NDEBUG
		_timeMetric.stop();
	#endif

		for (auto& child : children())
		{
			child->update();
		}
	}

	GameObject::GameObject(const std::string& p_name) :
	#ifndef NDEBUG
		_timeMetric(spk::Application::activeApplication()->profiler().metric<TimeMetric>("Object : " + p_name)),
	#endif
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
		activate();
	}

	GameObject::GameObject(const std::string& p_name, GameObject* p_parent) :
		GameObject(p_name)
	{
		if (p_parent != nullptr)
			p_parent->addChild(this);
	}

	GameObject::~GameObject()
	{
		
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

	void GameObject::addTag(const std::string& p_tag)
	{
		if (std::find(_tags.begin(), _tags.end(), p_tag) == _tags.end())
		{
			_tags.push_back(p_tag);
		}
	}
	
	void GameObject::removeTag(const std::string& p_tag)
	{
		_tags.erase(
			std::remove(_tags.begin(), _tags.end(), p_tag), 
			_tags.end());
	}
	
	const std::vector<std::string>& GameObject::tags() const
	{
		return (_tags);
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

	const std::vector<spk::GameComponent*>& GameObject::components() const
	{
		return (_components);
	}
}