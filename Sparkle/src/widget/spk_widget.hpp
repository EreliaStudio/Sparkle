#pragma once

#include "design_pattern/spk_activable_object.hpp"
#include "design_pattern/spk_tree_node.hpp"
#include "graphics/spk_viewport.hpp"

namespace spk
{
	/**
	 * @class IWidget
	 * @brief Defines a widget interface for UI elements, integrating activation state management, hierarchicalorganization, and viewport handling.
	 *
	 * This extends both ActivateObject for managing activation states (enabling/disabling functionality) and
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

		void render();
		void update();

		void resize(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);

		spk::Vector2Int _computeAbsoluteAnchor();
		void _computeViewport();

	public:
		IWidget(const std::string& p_name);
		IWidget(const std::string& p_name, IWidget* p_parent);

		~IWidget();

		void addChild(IWidget* p_children);
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
		void setDepth(const float& p_depth);

		void activateAll();
		void deactivateAll();

		const std::string& name() const;

		const float& depth() const;
		const spk::Vector2Int& anchor() const;
		const spk::Vector2UInt& size() const;
		const spk::Viewport& viewport() const;
	};
}