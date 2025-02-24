#pragma once

#include <algorithm>
#include <vector>
#include <stdexcept>
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	template <typename TType>
	class InherenceObject
	{
	public:
		using Parent = InherenceObject<TType>*;
		using Child = InherenceObject<TType>*;
		using ParentObject = spk::SafePointer<TType>;
		using ChildObject = spk::SafePointer<TType>;
		using ChildArray = std::vector<ChildObject>;

	private:
		Parent _parent;
		ChildArray _children;

	protected:
		InherenceObject() : _parent(nullptr)
		{
		}

	public:
		virtual ~InherenceObject()
		{
			for (ChildObject child : _children)
			{
				static_cast<Child>(child.get())->_parent = nullptr;
			}
			
			if (_parent != nullptr)
			{
				_parent->removeChild(this);
			}
		}

		InherenceObject(const InherenceObject&) = delete;
		InherenceObject& operator=(const InherenceObject&) = delete;

		InherenceObject(InherenceObject&& p_other) noexcept : _parent(nullptr)
		{
			transferFrom(std::move(p_other));
		}

		InherenceObject& operator=(InherenceObject&& p_other) noexcept
		{
			if (this != &p_other)
			{
				clearChildren();

				transferFrom(std::move(p_other));
			}
			return *this;
		}

		virtual void addChild(ChildObject p_child)
		{
			_children.push_back(p_child);
			static_cast<Child>(p_child.get())->_parent = static_cast<Child>(this);
		}

		virtual void removeChild(Child p_child)
		{
			auto it = std::find(_children.begin(), _children.end(), p_child);
			if (it == _children.end())
			{
				throw std::runtime_error("Child not found in children array");
			}
			_children.erase(it);
			p_child->_parent = nullptr;
		}

		virtual void removeChild(ChildObject p_child)
		{
			auto it = std::find(_children.begin(), _children.end(), p_child);
			if (it == _children.end())
			{
				throw std::runtime_error("Child not found in children array");
			}
			_children.erase(it);
			static_cast<Child>(p_child.get())->_parent = nullptr;
		}

		void transferChildren(Parent p_newParent)
		{
			for (ChildObject child : _children)
			{
				p_newParent->addChild(child);
			}
			_children.clear();
		}

		void clearChildren()
		{
			for (ChildObject& child : _children)
			{
				static_cast<Child>(child.get())->_parent = nullptr;
			}
			_children.clear();
		}

		ParentObject parent() const
		{
			return static_cast<TType*>(_parent);
		}

		ChildArray& children()
		{
			return _children;
		}

		const ChildArray& children() const
		{
			return _children;
		}
	};
}