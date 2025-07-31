#pragma once

#include "structure/design_pattern/spk_activable_object.hpp"
#include "structure/design_pattern/spk_identifiable_object.hpp"
#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/system/spk_boolean_enum.hpp"

#include "structure/system/event/spk_event.hpp"

#include "structure/engine/spk_transform.hpp"
#include <set>
#include <span>
#include <string>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class GameObject : public spk::InherenceObject<GameObject>, public spk::ActivableObject
	{
	public:
		using Contract = spk::ActivableObject::Contract;
		using Job = spk::ActivableObject::Job;

	private:
		std::wstring _name;
		std::set<std::wstring> _tags;

		int _priority;

		mutable std::mutex _componentMutex;
		mutable std::mutex _childMutex;

		bool _needChildSorting = false;
		bool _needComponentSorting = false;
		std::vector<std::unique_ptr<Component>> _components;
		spk::SafePointer<Transform> _transform;
		Transform::Contract _ownerTransformEditionContract;

		Contract _awakeContract;
		Contract _sleepContract;

		Contract constructAwakeContract();
		Contract constructSleepContract();

		template <typename T>
		void sortByPriority(std::vector<T> &p_container, bool &p_needSorting, std::mutex &p_mutex)
		{
			std::unique_lock<std::mutex> mutexLock(p_mutex);

			if (p_needSorting == false)
			{
				return;
			}

			std::sort(p_container.begin(),
					  p_container.end(),
					  [](const T &a, const T &b)
					  {
						  int priorityA = a->priority();
						  int priorityB = b->priority();

						  bool aNoPriority = (priorityA < 0);
						  bool bNoPriority = (priorityB < 0);

						  if (aNoPriority && bNoPriority)
						  {
							  return (false);
						  }

						  if (aNoPriority != bNoPriority)
						  {
							  return (!aNoPriority);
						  }

						  return (priorityA > priorityB);
					  });

			p_needSorting = false;
		}

	public:
		GameObject(const std::wstring &p_name, const spk::SafePointer<GameObject> &p_owner = nullptr);

		GameObject();

		GameObject(const GameObject &) = delete;
		GameObject &operator=(const GameObject &) = delete;

		~GameObject();

		void setName(const std::wstring &p_name);
		void setPriority(const int &p_priority);

		void addTag(const std::wstring &p_tag);
		void removeTag(const std::wstring &p_tag);
		void clearTags();
		bool containTag(const std::wstring &p_tag) const;

		const std::wstring &name() const;
		const std::set<std::wstring> &tags() const;
		const spk::Vector3 &position() const;
		int priority() const;
		Transform &transform();
		const Transform &transform() const;

		void removeAllComponents();

		template <typename TComponentType, typename... TArgs>
		spk::SafePointer<TComponentType> addComponent(TArgs &&...p_args)
		{
			static_assert(std::is_base_of_v<Component, TComponentType>, "TComponentType must inherit from Component");
			TComponentType *newComponent = new TComponentType(std::forward<TArgs>(p_args)...);

			{
				std::unique_lock<std::mutex> lock(_componentMutex);

				_components.emplace_back(std::unique_ptr<Component>(newComponent));
			}

			newComponent->_owner = this;

			newComponent->start();
			newComponent->activate();

			return (newComponent);
		}

		void removeComponent(const std::wstring &p_name);

		template <typename TComponentType>
		spk::SafePointer<TComponentType> getComponent(const std::wstring &p_name = L"")
		{
			std::unique_lock<std::mutex> lock(_componentMutex);

			for (std::unique_ptr<Component> &component : _components)
			{
				TComponentType *castedComponent = dynamic_cast<TComponentType *>(component.get());
				if (castedComponent != nullptr)
				{
					if (p_name.empty() || castedComponent->name() == p_name)
					{
						return (castedComponent);
					}
				}
			}

			GENERATE_ERROR("Component of specified type and name not found.");
		}

		template <typename TComponentType>
		std::vector<spk::SafePointer<TComponentType>> getComponents()
		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			std::vector<spk::SafePointer<TComponentType>> result;

			for (std::unique_ptr<Component> &component : _components)
			{
				TComponentType *castedComponent = dynamic_cast<TComponentType *>(component.get());
				if (castedComponent != nullptr)
				{
					result.emplace_back(castedComponent);
				}
			}

			return (result);
		}

		template <typename TComponentType>
		const spk::SafePointer<TComponentType> getComponent(const std::wstring &p_name = L"") const
		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (const std::unique_ptr<Component> &component : _components)
			{
				TComponentType *castedComponent = dynamic_cast<TComponentType *>(component.get());
				if (castedComponent != nullptr)
				{
					if (p_name.empty() || castedComponent->name() == p_name)
					{
						return (castedComponent);
					}
				}
			}

			GENERATE_ERROR("Component of specified type and name not found.");
		}

		template <typename TComponentType>
		std::vector<spk::SafePointer<const TComponentType>> getComponents() const
		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			std::vector<spk::SafePointer<const TComponentType>> result;

			for (const std::unique_ptr<Component> &component : _components)
			{
				const TComponentType *castedComponent = dynamic_cast<const TComponentType *>(component.get());
				if (castedComponent != nullptr)
				{
					result.emplace_back(castedComponent);
				}
			}

			return (result);
		}

		virtual void onPaintEvent(spk::PaintEvent &p_event) final;
		virtual void onUpdateEvent(spk::UpdateEvent &p_event) final;
		virtual void onKeyboardEvent(spk::KeyboardEvent &p_event) final;
		virtual void onMouseEvent(spk::MouseEvent &p_event) final;
		virtual void onControllerEvent(spk::ControllerEvent &p_event) final;
		virtual void onTimerEvent(spk::TimerEvent &p_event) final;

		void sortChildren();
		void sortComponent();

		spk::SafePointer<GameObject> getChild(const std::wstring &p_name);
		spk::SafePointer<const GameObject> getChild(const std::wstring &p_name) const;
		std::vector<spk::SafePointer<GameObject>> getChildren(const std::wstring &p_name);
		std::vector<spk::SafePointer<const GameObject>> getChildren(const std::wstring &p_name) const;
		bool contains(const std::wstring &p_name) const;
		size_t count(const std::wstring &p_name) const;

		spk::SafePointer<GameObject> getChildByTag(const std::wstring &p_tag);
		spk::SafePointer<const GameObject> getChildByTag(const std::wstring &p_tag) const;
		std::vector<spk::SafePointer<GameObject>> getChildrenByTag(const std::wstring &p_tag);
		std::vector<spk::SafePointer<const GameObject>> getChildrenByTag(const std::wstring &p_tag) const;
		bool containsTag(const std::wstring &p_tag) const;
		size_t countTag(const std::wstring &p_tag) const;

		spk::SafePointer<GameObject> getChildByTags(const std::span<const std::wstring> &p_tags,
													spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		spk::SafePointer<const GameObject> getChildByTags(const std::span<const std::wstring> &p_tags,
														  spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		std::vector<spk::SafePointer<GameObject>> getChildrenByTags(const std::span<const std::wstring> &p_tags,
																	spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		std::vector<spk::SafePointer<const GameObject>> getChildrenByTags(const std::span<const std::wstring> &p_tags,
																		  spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		bool containsTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		size_t countTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
	};
}