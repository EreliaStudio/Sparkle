#pragma once

#include "design_pattern/spk_activable_object.hpp"
#include "design_pattern/spk_tree_node.hpp"
#include "graphics/spk_viewport.hpp"

namespace spk
{
	class IWidget : public ActivateObject, public TreeNode<IWidget>
	{
		friend class Application;

	private:
		static inline IWidget* defaultParent = nullptr;

		std::string _name;

		bool _needGeometryChange = false;

		spk::Vector2Int _anchor;
		spk::Vector2 _anchorRatio;
		spk::Vector2UInt _size;
		spk::Vector2 _sizeRatio;

		float _depth;

		spk::Viewport _viewport;

		virtual void _onGeometryChange() = 0;

		virtual void _onRender() = 0;

		virtual void _onUpdate() = 0;

		void render()
		{
			if (_needGeometryChange == true)
			{
				_computeViewport();
				_onGeometryChange();
				_needGeometryChange = false;
			}

			_onRender();

			for (auto& child : childrens())
			{
				if (child->isActive() == true)
				{
					_viewport.activate();
					child->render();
				}
			}
		}

		void update()
		{
			_onUpdate();

			for (auto& child : childrens())
			{
				if (child->isActive() == true)
				{
					child->update();
				}
			}
		}

		void resize(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
		{
			_anchor = p_anchor;
			_size = p_size;
			_needGeometryChange = true;
			
			for (auto& child : childrens())
			{
				child->resize(_size * child->_anchorRatio, _size * child->_sizeRatio);
			}
			_onGeometryChange();
		}

		spk::Vector2Int _computeAbsoluteAnchor()
		{
			spk::Vector2Int result = 0;
			const IWidget* tmp = this;

			while (tmp->parent() != nullptr)
			{
				result += tmp->anchor();
				tmp = tmp->parent();
			}

			return (result);
		}

		void _computeViewport()
		{
			spk::Vector2Int topLeft = _computeAbsoluteAnchor();
			spk::Vector2Int rightDown = size() + topLeft;

			if (parent() != nullptr)
				topLeft = spk::Vector2Int::max(topLeft, parent()->viewport().anchor());

			if (parent() != nullptr)
				rightDown = spk::Vector2Int::min(rightDown, parent()->viewport().anchor() + parent()->viewport().size());

			_viewport.setGeometry(topLeft, rightDown - topLeft);
		}

	public:
		IWidget(const std::string& p_name) :
			_name(p_name),
			_depth(0)
		{
			if (defaultParent != nullptr)
				defaultParent->addChild(this);
		}

		IWidget(const std::string& p_name, IWidget* p_parent) :
			_name(p_name),
			_depth(0)
		{
			p_parent->addChild(this);
		}

		~IWidget()
		{
			if (defaultParent != nullptr)
			{
				transferChildrens(defaultParent);
			}
		}

		void addChild(IWidget* p_children)
		{
			TreeNode<IWidget>::addChild(p_children);
			p_children->setDepth(depth() + 1);
		}

		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
		{
			_anchor = p_anchor;
			_size = p_size;

			_anchorRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_anchor) / static_cast<spk::Vector2>(parent()->size()) : 0);
			_sizeRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_size) / static_cast<spk::Vector2>(parent()->size()) : 1);

			_needGeometryChange = true;
		}

		void setDepth(const float& p_depth)
		{
			_depth = p_depth;
		}

		const std::string& name() const
		{
			return (_name);
		}

		const spk::Vector2Int& anchor() const
		{
			return (_anchor);
		}

		const float& depth() const
		{
			return (_depth);
		}

		const spk::Vector2UInt& size() const
		{
			return (_size);
		}

		const spk::Viewport& viewport() const
		{
			return (_viewport);
		}
	};
}