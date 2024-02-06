#pragma once

#include "design_pattern/spk_activable_object.hpp"
#include "design_pattern/spk_tree_node.hpp"
#include "graphics/spk_viewport.hpp"

namespace spk
{
	/**
	 * @brief Defines a widget interface for UI elements, integrating activation state management, hierarchicalorganization, and viewport handling.
	 *
	 * IWidget extends both ActivateObject for managing activation states (enabling/disabling functionality) and
	 * TreeNode for hierarchical structuring, allowing widgets to be organized in a tree structure.
	 * This designsupports complex UI hierarchies with nested widgets, facilitating the development of layered
	 * and structured user interfaces.
	 *
	 * Each widget is associated with a viewport, automatically managed based on its geometry and the
	 * hierarchical structure.
	 * The viewport of a widget is activated upon rendering its children, ensuring that rendering operations
	 * are confined to the widget's designated area. This mechanism is crucial for rendering efficiency and visual
	 * correctness, especially in complex UI systems with overlapping elements.
	 *
	 * Widgets are designed to be extensible, requiring the implementation of key lifecycle
	 * methods (_onGeometryChange, _onRender, and _onUpdate) to define their behavior, appearance,
	 * and response to events or changes in state. The class also provides methods for resizing andgeometry
	 * management, allowing widgets to adapt to layout changes dynamically.
	 *
	 * Usage example:
	 * @code
	 * class MyWidget : public spk::IWidget {
	 *     MyWidget(const std::string& name) : spk::IWidget(name) {}
	 *     void _onGeometryChange() override {
	 *         // Custom geometry change handling
	 *     }
	 *     void _onRender() override {
	 *         // Rendering code for this widget
	 *     }
	 *     void _onUpdate() override {
	 *         // Update logic for this widget
	 *     }
	 * };
	 * 
	 * spk::Application app("MyApplication", spk::Vector2UInt(800, 640));
	 * 	
	 * MyWidget a("MyFirstWidget");
	 * a.setGeometry(spk::Vector2Int(0, 0), app.size());
	 * a.activate();
	 * 
	 * return (app.run());
	 * @endcode
	 *
	 * @note If no parent are set for a widget, this widget will be concidered as child of the default widget of the application.
	 * @note The viewport of a IWidget will automatically be activated upon rendering its children, as mentioned in the class documentation.
	 */
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