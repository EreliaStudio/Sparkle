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
		std::vector<TType*> _children;

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new instance of the TreeNode class without any parent or children.
		 */
		TreeNode()
		{
			_parent = nullptr;
		}

		
		/**
		 * @brief Destructor.
		 * 
		 * Cleans up by detaching all children from this node.
		 * Children will have their parent pointer set to nullptr.
		 */
		~TreeNode()
		{
			for (const auto& child : _children)
			{
				child->_parent = nullptr;
			}
			_parent->removeChild(this);
		}

		/**
		 * @brief Gets the parent of this node.
		 * 
		 * @return A pointer to the parent node, or nullptr if this node is the root of its tree.
		 */
		TType* parent()
		{
			return (_parent);
		}

		/**
		 * @brief Gets the parent of this node.
		 * 
		 * @return A const pointer to the parent node, or nullptr if this node is the root of its tree.
		 */
		const TType* parent() const
		{
			return (_parent);
		}

		/**
		 * @brief Gets the children of this node.
		 * 
		 * @return A vector of pointers to the child nodes.
		 */
		const std::vector<TType*> children() const
		{
			return (_children);
		}

		/**
		 * @brief Adds a child node to this node.
		 * 
		 * If the child already has a parent, it is first removed from its current parent. Then, the child is added to this node's
		 * list of children, and this node is set as the new parent of the child.
		 * 
		 * @param p_children Pointer to the child node to add. It must not be nullptr.
		 */
		void addChild(TType* p_children)
		{
			if (p_children->_parent != nullptr)
				p_children->_parent->removeChild(p_children);

			_children.push_back(p_children);

			p_children->_parent = static_cast<TType*>(this);
		}
				
		/**
		 * @brief Removes a specific child from this node.
		 * 
		 * Searches for the given child in the list of children and removes it if found. This function does not delete the child node;
		 * it only removes the association between this node and the child.
		 * 
		 * @param p_child Pointer to the child node to remove.
		 */
		void removeChild(TType* p_child)
		{
			auto it = std::find(_children.begin(), _children.end(), p_child);
			if (it != _children.end())
			{
				_children.erase(it);
			}
		}

		/**
		 * @brief Transfers all children of this node to another parent node.
		 * 
		 * All children of this node are moved to the specified new parent. This node will have no children after this operation.
		 * 
		 * @param p_newParent Pointer to the new parent node. It must not be nullptr.
		 */
		void transferChildrens(TType* p_newParent)
		{
			for (const auto& child : _children)
			{
				p_newParent->addChild(child);
			}
		}
	};
}