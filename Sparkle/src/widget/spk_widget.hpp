#pragma once

#include <atomic>
#include <cassert>

#include "design_pattern/spk_activable_object.hpp"
#include "design_pattern/spk_tree_node.hpp"
#include "graphics/spk_viewport.hpp"
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	/**
	 * @class Widget
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
	 * class MyWidget : public spk::Widget {
	 *	 MyWidget(const std::string& name) : spk::Widget(name) {}
	 *	 void _onGeometryChange() override {
	 *		 // Custom geometry change handling
	 *	 }
	 *	 void _onRender() override {
	 *		 // Rendering code for this widget
	 *	 }
	 *	 void _onUpdate() override {
	 *		 // Update logic for this widget
	 *	 }
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
	 * @note The viewport of a Widget will automatically be activated upon rendering its children, as mentioned in the class documentation.
	 */
	class Widget : public ActivateObject, public TreeNode<Widget>
	{
		friend class Application;

	private:
		static inline Widget* defaultParent = nullptr;

		std::string _name;

		bool _needGeometryUpdate = false;

		spk::Vector2Int _anchor;
		spk::Vector2 _anchorRatio;
		spk::Vector2UInt _size;
		spk::Vector2 _sizeRatio;
		std::vector<std::unique_ptr<Widget>> _ownedChildren;

		float _layer;

		spk::Viewport _viewport;

#ifndef NDEBUG
		TimeMetric& _timeMetric;
#endif

	protected:
		/**
		 * @brief Virtual method for rendering the widget.
		 *
		 * This method is called during the rendering phase of the widget lifecycle. Derived classes should override
		 * this method to implement the drawing logic specific to the widget. This can include rendering shapes,
		 * text, images, or any other visual elements associated with the widget. Implementations should make use of
		 * the widget's viewport to confine rendering to the designated area.
		 */
		virtual void _onRender();

		/**
		 * @brief Virtual method called during the update phase of the widget lifecycle.
		 *
		 * This method is called periodically to allow the widget to update its state. This can include processing
		 * input events, animating properties, or making any other state changes that need to occur over time. Derived
		 * classes should override this method to implement such behavior, ensuring that the widget remains responsive
		 * and dynamic.
		 */
		virtual void _onUpdate();

		/**
		 * @brief Propagate geometry changes to components.
		 *
		 * This method is called in reaction to setGeometry.
		 */
		virtual void _onGeometryChange();

	private:
		void render();
		void update();
		void resize(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);

	private:
		spk::Vector2Int _computeAbsoluteAnchor();
		void _computeViewport();

	public:
		/**
		 * Constructor for creating a widget without a name and set the parent widget.
		 * @param p_parent A pointer to the parent widget. This widget will be added as a child of the given parent.
		 */
		Widget(Widget* p_parent);

		/**
		 * Constructor for creating a widget with a specified name.
		 * @param p_name The name of the widget. This is used for identification and debugging purposes.
		 */
		Widget(const std::string& p_name);

		/**
		 * Constructor for creating a widget with a specified name and parent widget.
		 * @param p_name The name of the widget. This is used for identification and debugging purposes.
		 * @param p_parent A pointer to the parent widget. This widget will be added as a child of the given parent.
		 */
		Widget(const std::string& p_name, Widget* p_parent);

		/**
		 * @brief Construct a widget using a JSON object as input
		 * @param p_object The JSON from where widget information will be taken from
		 * @param p_parent A pointer to the parent widget. This widget will be added as a child of the given parent.
		 * 
		 * @note JSON format : 
		 * {
		 * "Name":"MyWidgetName",
		 * "WidgetType":"MyWidget",
		 * "Parent":"MyParentWidget",
		 * "Active":false,
		 * "MyCustomInformation":15,
		 * "MyCustomString":"This is a test"
		 * }
		*/
		Widget(const spk::JSON::Object& p_object, Widget* p_parent);

		/**
		 * Destructor for the widget. Cleans up resources and ensures proper deactivation of the widget and its children.
		 */
		~Widget();

		/**
		 * Reset the name of the widget to a desired value
		 * @param p_name The new name of the widget
		*/
		void rename(const std::string& p_name);

		/**
		 * Adds a child widget to this widget. The child widget will be hierarchically managed and rendered within this widget's viewport.
		 * @param p_child A pointer to the widget to be added as a child.
		 */
		void addChild(Widget* p_child);

		/**
		 * @brief Create a child widget of type TChild.
		 * This parent is then responsible for the lifetime of children created this way.
		 *
		 * @param p_args Arguments used for the TChild constructor.
		 * @return A temporary pointer to the child, without ownership.
		 */
		template <class TChild, typename... Args>
		TChild* makeChild(Args&&... p_args)
		{
			TChild* child = new TChild(std::forward<Args>(p_args)...);
			addChild(child); 
			_ownedChildren.push_back(std::unique_ptr<Widget>(child));
			return child;
		}

		/**
		 * @brief Create a child widget of type TChild and activate it.
		 * This parent is then responsible for the lifetime of children created this way.
		 *
		 * @param p_args Arguments used for the TChild constructor.
		 * @return A temporary pointer to the activated child, without ownership.
		 */
		template <class TChild, typename... Args>
		TChild* makeActiveChild(Args&&... p_args)
		{
			TChild* child = makeChild<TChild>(std::forward<Args>(p_args)...);
			child->activate();
			return child;
		}

		/**
		 * @brief Retrieve a child widget using its name
		 * @param p_name The name of the targeted widget
		 * @return The widget if found, nullptr if not
		*/
		Widget* findChild(const std::string& p_name)
		{
			for (auto& child : children())
			{
				if (child->name() == p_name)
					return (child);
			}
			
			for (auto& child : children())
			{
				Widget* childResult = child->findChild(p_name);
				if (childResult != nullptr)
					return (childResult);
			}

			return (nullptr);
		}

		/**
		 * Sets the geometry of the widget, specifying its anchor point and size.
		 * @param p_anchor The anchor point of the widget in its parent's coordinate system.
		 * @param p_size The size of the widget.
		 */
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);

		/**
		 * Sets the layer of the widget, affecting its rendering order relative to its siblings.
		 * @param p_layer The layer value of the widget. Lower values are rendered behind higher values.
		 */
		void setLayer(const float& p_layer);

		/**
		 * Activates this widget and all its child widgets recursively, enabling their functionality and making them eligible for rendering and updates.
		 */
		void activateAll();

		/**
		 * Deactivates this widget and all its child widgets recursively, disabling their functionality and excluding them from rendering and updates.
		 */
		void deactivateAll();

		/**
		 * @brief Tests if the given coordinates in cartesian space is in this widget's viewport.
		 * This method should be overriden if another behavior is needed.
		 * @param p_coord The point that needs to be hit tested.
		 */
		virtual bool hitTest(const Vector2& p_coord);

		/**
		 * Gets the name of the widget.
		 * @return A constant reference to the widget's name.
		 */
		const std::string& name() const;

		/**
		 * Sets the name of the widget.
		 */
		void setName(const std::string& p_name);

		/**
		 * Gets the layer of the widget.
		 * @return A constant reference to the widget's layer value.
		 */
		const float& layer() const;

		/**
		 * Gets the anchor point of the widget.
		 * @return A constant reference to the widget's anchor point.
		 */
		const spk::Vector2Int& anchor() const;

		/**
		 * Gets the size of the widget.
		 * @return A constant reference to the widget's size.
		 */
		const spk::Vector2UInt& size() const;

		/**
		 * Gets the viewport of the widget.
		 * @return A constant reference to the widget's viewport. The viewport defines the area of the screen where the widget is rendered.
		 */
		const spk::Viewport& viewport() const;
	};
}
