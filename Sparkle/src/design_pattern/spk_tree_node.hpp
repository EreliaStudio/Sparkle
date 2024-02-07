#pragma once

#include <vector>

namespace spk
{
	/**
	 * @class TreeNode
	 * 
	 * @tparam TType The type of the tree nodes, typically a derived class.
	 * 
	 * @brief A generic tree node class for building hierarchical structures.
	 * 
	 * The TreeNode class represents a node in a hierarchical structure, such as a tree. It provides
	 * functionality for managing parent-child relationships within the structure. Each node can have
	 * only one parent and zero or more children.
	 * 
	 * This class is designed to be a base class for creating hierarchical data structures in various
	 * applications, including scene graphs, organization charts, or other tree-based data representations.
	 * 
	 * Usage example:
	 * @code
	 * spk::TreeNode<MyTreeNodeType> parentNode;
	 * spk::TreeNode<MyTreeNodeType> childNode1;
	 * spk::TreeNode<MyTreeNodeType> childNode2;
	 * 
	 * parentNode.addChild(&childNode1);
	 * parentNode.addChild(&childNode2);
	 * @endcode
	 */
	template<typename TType>
	class TreeNode
	{
	private:
		TType* _parent;
		std::vector<TType*> _childrens;

	protected:
		void removeChild(TType* p_child)
		{
			auto it = std::find(_childrens.begin(), _childrens.end(), p_child);
			if (it != _childrens.end())
			{
				_childrens.erase(it);
			}
		}

		void transferChildrens(TType* p_newParent)
		{
			for (const auto& child : _childrens)
			{
				p_newParent->addChild(child);
			}
		}

	public:
		TreeNode()
		{
			_parent = nullptr;
		}

		~TreeNode()
		{
			for (const auto& child : _childrens)
			{
				child->_parent = nullptr;
			}
		}

		const TType* parent() const
		{
			return (_parent);
		}

		const std::vector<TType*> childrens() const
		{
			return (_childrens);
		}
		
		void addChild(TType* p_children)
		{
			if (p_children->_parent != nullptr)
				p_children->_parent->removeChild(p_children);

			_childrens.push_back(p_children);

			p_children->_parent = static_cast<TType*>(this);
		}
	};
}