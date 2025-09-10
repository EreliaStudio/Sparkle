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
	class GameEngine;

	class Entity : public spk::InherenceObject<Entity>, public spk::ActivableObject
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
		GameEngine *_engine = nullptr;

		template <typename T>
		void _sortByPriority(std::vector<T> &p_container, bool &p_needSorting, std::mutex &p_mutex)
		{
			std::unique_lock<std::mutex> mutexLock(p_mutex);

			if (p_needSorting == false)
			{
				return;
			}

			std::sort(
				p_container.begin(),
				p_container.end(),
				[](const T &p_a, const T &p_b)
				{
					int priorityA = p_a->priority();
					int priorityB = p_b->priority();

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
		Entity(const std::wstring &p_name, const spk::SafePointer<Entity> &p_owner = nullptr);

		Entity();

		Entity(const Entity &) = delete;
		Entity &operator=(const Entity &) = delete;

		~Entity() override;

		virtual void setName(const std::wstring &p_name);
		void setPriority(const int &p_priority);

		void addTag(const std::wstring &p_tag);
		void removeTag(const std::wstring &p_tag);
		void clearTags();
		bool containTag(const std::wstring &p_tag) const;

		void setEngine(GameEngine *p_engine);
		spk::SafePointer<GameEngine> engine();
		spk::SafePointer<const GameEngine> engine() const;

		const std::wstring &name() const;
		const std::set<std::wstring> &tags() const;
		const spk::Vector3 &position() const;
		int priority() const;
		Transform &transform();
		const Transform &transform() const;

		void removeAllComponents();

		void addChild(spk::SafePointer<Entity> p_child) override;
		void removeChild(spk::SafePointer<Entity> p_child) override;
		void removeChild(InherenceObject<Entity>::Child p_child) override;
		void clearChildren();

		template <typename TComponentType, typename... TArgs>
		spk::SafePointer<TComponentType> addComponent(TArgs &&...p_args)
		{
			static_assert(std::is_base_of_v<Component, TComponentType>, "TComponentType must inherit from Component");
			std::unique_ptr<TComponentType> newUniquePtr = std::make_unique<TComponentType>(std::forward<TArgs>(p_args)...);
			TComponentType *newComponent = newUniquePtr.get();

			{
				std::unique_lock<std::mutex> lock(_componentMutex);

				_components.emplace_back(std::move(newUniquePtr));
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

			return (nullptr);
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

			return (nullptr);
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

		virtual void onGeometryChange(const spk::Geometry2D &p_geometry) final;
		virtual void onPaintEvent(spk::PaintEvent &p_event) final;
		virtual void onUpdateEvent(spk::UpdateEvent &p_event) final;
		virtual void onKeyboardEvent(spk::KeyboardEvent &p_event) final;
		virtual void onMouseEvent(spk::MouseEvent &p_event) final;
		virtual void onControllerEvent(spk::ControllerEvent &p_event) final;
		virtual void onTimerEvent(spk::TimerEvent &p_event) final;

		void sortChildren();
		void sortComponent();

		spk::SafePointer<Entity> getChild(const std::wstring &p_name);
		spk::SafePointer<const Entity> getChild(const std::wstring &p_name) const;
		std::vector<spk::SafePointer<Entity>> getChildren(const std::wstring &p_name);
		std::vector<spk::SafePointer<const Entity>> getChildren(const std::wstring &p_name) const;
		bool contains(const std::wstring &p_name) const;
		size_t count(const std::wstring &p_name) const;

		spk::SafePointer<Entity> getChildByTag(const std::wstring &p_tag);
		spk::SafePointer<const Entity> getChildByTag(const std::wstring &p_tag) const;
		std::vector<spk::SafePointer<Entity>> getChildrenByTag(const std::wstring &p_tag);
		std::vector<spk::SafePointer<const Entity>> getChildrenByTag(const std::wstring &p_tag) const;
		bool containsTag(const std::wstring &p_tag) const;
		size_t countTag(const std::wstring &p_tag) const;

		spk::SafePointer<Entity> getChildByTags(
			const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		spk::SafePointer<const Entity> getChildByTags(
			const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		std::vector<spk::SafePointer<Entity>> getChildrenByTags(
			const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND);
		std::vector<spk::SafePointer<const Entity>> getChildrenByTags(
			const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		bool containsTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
		size_t countTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator = spk::BinaryOperator::AND) const;
	};
}