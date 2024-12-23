#pragma once

#include "structure/spk_safe_pointer.hpp"

#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/event/spk_event.hpp"

#include "structure/graphics/spk_pipeline.hpp"

namespace spk
{
	class Window;

	class Widget :
		public spk::InherenceObject<Widget>, //An object responsible to hold relationship between "child" objects and a unique parent
		public spk::ActivableObject //An object who must contain a state as boolean, with a method activate and deactivate. It must also contain a set of callback where users can add callback to activation and deactivation
	{
		friend class Window;
	private:
		std::wstring _name;
		spk::SafePointer<Widget> _parent;
		static inline spk::Pipeline::Constant* WidgetConstants = nullptr;

		std::vector<Widget*> _managedChildren;

		bool _needGeometryChange = true;
		spk::Vector2 _anchorRatio;
		spk::Vector2 _sizeRatio;
		spk::Geometry2D _geometry;
		spk::Viewport _viewport;
		
		float _layer;

		virtual void _onGeometryChange();
		virtual void _onPaintEvent(const spk::PaintEvent& p_event);
		virtual void _onUpdateEvent(const spk::UpdateEvent& p_event);
		virtual void _onKeyboardEvent(const spk::KeyboardEvent& p_event);
		virtual void _onMouseEvent(const spk::MouseEvent& p_event);
		virtual void _onControllerEvent(const spk::ControllerEvent& p_event);

		spk::Geometry2D::Point _computeAbsoluteAnchor();
		void _computeViewport();

		void _computeRatio();
		void _resize();

	public:
		Widget(const std::wstring& p_name);
		Widget(const std::wstring& p_name, spk::SafePointer<Widget> p_parent);

		~Widget();

		const std::wstring& name() const;

		void addChild(spk::SafePointer<Widget> p_child) override;

		template<typename TChildType, typename... TArgs>
		spk::SafePointer<TChildType> makeChild(TArgs&&... p_args)
		{
			TChildType* newChild = new TChildType(std::forward<TArgs>(p_args)...);

			addChild(newChild);

			_managedChildren.push_back(newChild);

			return (spk::SafePointer<TChildType>(newChild));
		}

		void setLayer(const float& p_layer);
		const float& layer() const;

		void forceGeometryChange(const Geometry2D& p_geometry);
		void setGeometry(const Geometry2D& p_geometry);
		void forceGeometryChange(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size);
		void updateGeometry();
		void requireGeometryUpdate();

		const Geometry2D& geometry() const;
		const Viewport& viewport() const;

		void onPaintEvent(const spk::PaintEvent& p_event);
		void onUpdateEvent(const spk::UpdateEvent& p_event);
		void onKeyboardEvent(const spk::KeyboardEvent& p_event);
		void onMouseEvent(const spk::MouseEvent& p_event);
		void onControllerEvent(const spk::ControllerEvent& p_event);
	};
}