#pragma once

#include <vector>

namespace spk
{
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