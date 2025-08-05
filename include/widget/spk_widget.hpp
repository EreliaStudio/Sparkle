#pragma once

#include "structure/spk_safe_pointer.hpp"

#include "structure/design_pattern/spk_activable_object.hpp"
#include "structure/design_pattern/spk_inherence_object.hpp"

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/event/spk_event.hpp"

#include "structure/graphics/texture/spk_font.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"

#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"
#include "structure/graphics/painter/spk_texture_painter.hpp"

namespace spk
{
	class Window;

	class Widget : public spk::InherenceObject<Widget>, // An object responsible to hold relationship between "child" objects and a unique parent
				   public spk::ActivableObject // An object who must contain a state as boolean, with a method activate and deactivate. It must also
											   // contain a set of callback where users can add callback to activation and deactivation
	{
		friend class Window;

	private:
		static spk::SpriteSheet _defaultIconset;
		static spk::Font _defaultFont;
		static spk::SpriteSheet _defaultNineSlice;

	public:
		static spk::SafePointer<const spk::SpriteSheet> defaultIconset();
		static spk::SafePointer<spk::Font> defaultFont();
		static spk::SafePointer<const spk::SpriteSheet> defaultNineSlice();

		enum class FocusType
		{
			KeyboardFocus = 0,
			MouseFocus = 1,
			ControllerFocus = 2
		};

		static spk::SafePointer<Widget> focusedWidget(FocusType p_focusType = FocusType::KeyboardFocus);

	protected:
		void requestPaint();

	private:
		static inline spk::SafePointer<Widget> _focusedWidgets[3] = {nullptr, nullptr, nullptr};

		std::wstring _name;
		spk::SafePointer<Widget> _parent;
		std::vector<Widget *> _managedChildren;

		bool _needGeometryChange = true;
		spk::Vector2 _anchorRatio;
		spk::Vector2 _sizeRatio;
		spk::Geometry2D _geometry;
		spk::Viewport _viewport;

		spk::OpenGL::FrameBufferObject _frameBufferObject;

		spk::TexturePainter _textureRenderer;

		spk::Vector2UInt _minimalSize = {0, 0};
		spk::Vector2UInt _maximalSize = {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};

		bool _requestedPaint = false;

		float _layer;

	protected:
		virtual void _onGeometryChange();
		virtual void _onPaintEvent(spk::PaintEvent &p_event);
		virtual void _onUpdateEvent(spk::UpdateEvent &p_event);
		virtual void _onKeyboardEvent(spk::KeyboardEvent &p_event);
		virtual void _onMouseEvent(spk::MouseEvent &p_event);
		virtual void _onControllerEvent(spk::ControllerEvent &p_event);
		virtual void _onTimerEvent(spk::TimerEvent &p_event);

	private:
		void _computeViewport();

		void _applyGeometryChange();

		void _computeRatio();
		void _applyResize();

		void _sortChildByLayer();

	public:
		Widget(const std::wstring &p_name);
		Widget(const std::wstring &p_name, spk::SafePointer<Widget> p_parent);

		~Widget();

		const std::wstring &name() const;
		void setName(const std::wstring &p_name);

		void addChild(spk::SafePointer<Widget> p_child) override;

		spk::SafePointer<const spk::OpenGL::FrameBufferObject> frameBufferObject() const;

		void renderAsPNJ(const std::filesystem::path &p_path);

		template <typename TChildType, typename... TArgs>
		spk::SafePointer<TChildType> makeChild(TArgs &&...p_args)
		{
			TChildType *newChild = new TChildType(std::forward<TArgs>(p_args)...);

			addChild(newChild);

			_managedChildren.push_back(newChild);

			return (spk::SafePointer<TChildType>(newChild));
		}

		bool isPointed(const spk::Vector2Int &p_pointerPosition) const;
		bool isPointed(const spk::Mouse &p_mouse) const;
		bool isPointed(const spk::SafePointer<const spk::Mouse> &p_mouse) const;

		void setLayer(const float &p_layer);
		const float &layer() const;

		void takeFocus(FocusType p_focusType);
		void releaseFocus(FocusType p_focusType);
		bool hasFocus(FocusType p_focusType);

		void takeFocus();
		void releaseFocus();

		spk::Geometry2D::Point absoluteAnchor();
		virtual void forceGeometryChange(const Geometry2D &p_geometry);
		virtual void place(const spk::Vector2Int &p_delta);
		virtual void move(const spk::Vector2Int &p_delta);
		virtual void setGeometry(const Geometry2D &p_geometry);
		virtual void forceGeometryChange(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size);
		virtual void setGeometry(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size);
		void applyGeometryChange();
		void requireGeometryUpdate();

		virtual void setMinimalSize(const spk::Vector2UInt &p_size);
		virtual void setMaximalSize(const spk::Vector2UInt &p_size);
		virtual spk::Vector2UInt minimalSize() const;
		virtual spk::Vector2UInt maximalSize() const;

		const Geometry2D &geometry() const;
		const Viewport &viewport() const;

		virtual void onPaintEvent(spk::PaintEvent &p_event) final;
		virtual void onUpdateEvent(spk::UpdateEvent &p_event) final;
		virtual void onKeyboardEvent(spk::KeyboardEvent &p_event) final;
		virtual void onMouseEvent(spk::MouseEvent &p_event) final;
		virtual void onControllerEvent(spk::ControllerEvent &p_event) final;
		virtual void onTimerEvent(spk::TimerEvent &p_event) final;
	};
}