#pragma once

#include <string_view>
#include <unordered_map>

#include "activable_trait.hpp"
#include "cached_data.hpp"
#include "inherence_trait.hpp"
#include "name_trait.hpp"
#include "record.hpp"
#include "rect2d.hpp"
#include "render_snapshot.hpp"
#include "resizeable_trait.hpp"
#include "view_region.hpp"

namespace spk
{
	struct UpdateContext;
	class Widget;

	struct WidgetChildComparator
	{
		bool operator()(const Widget *lhs, const Widget *rhs) const;
	};

	class Widget : public spk::NameTrait,
				   public spk::InherenceTrait<Widget, WidgetChildComparator>,
				   public spk::ResizeableTrait,
				   public spk::ActivableTrait
	{
	public:
		static inline spk::RenderPass::Key OverlayKey = {
			.name = "sparkle.Overlay",
			.order = 0};

		using ZOrder = float;

	private:
		using Inherence = InherenceTrait<Widget, WidgetChildComparator>;

		Inherence::OnParentEditionContract _onParentEditedContract;
		std::unordered_map<Widget *, ResizeableTrait::Contract> _childSizeHintEditionContracts;
		ZOrder _zOrder = 0;
		spk::CachedData<ZOrder> _absoluteZOrder;
		spk::Rect2D _geometry{};
		spk::Vector2 _anchorRatio{0.0f, 0.0f};
		spk::Vector2 _sizeRatio{1.0f, 1.0f};
		spk::CachedData<ViewRegion> _viewRegion;

		void _invalidateViewRegion();
		void _invalidateAbsoluteZOrder();
		void _computeRatio();
		[[nodiscard]] spk::Rect2D _geometryFromRatio(const Widget &child) const;
		void _resize(const spk::Rect2D &geometry);
		void _onChildSizeHintEdition();

		void _onChildAdded(Widget *child) override final;
		void _onChildRemoved(Widget *child) override final;

		template <typename TEvent>
		void _propagate(TEvent &event, void (Widget::*handler)(TEvent &), std::string_view eventName);

		void _buildViewRegionCommands(spk::RenderSnapshot::Builder &builder);

		virtual void _updateState(UpdateContext &context);
		virtual void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		virtual void _updateSizeHint();
		virtual void _onGeometryChange();

		virtual void _onWindowResizedEvent(WindowResizedEvent &event);
		virtual void _onWindowMovedEvent(WindowMovedEvent &event);
		virtual void _onWindowFocusGainedEvent(WindowFocusGainedEvent &event);
		virtual void _onWindowFocusLostEvent(WindowFocusLostEvent &event);
		virtual void _onMouseEnteredEvent(MouseEnteredEvent &event);
		virtual void _onMouseLeftEvent(MouseLeftEvent &event);
		virtual void _onMouseMovedEvent(MouseMovedEvent &event);
		virtual void _onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event);
		virtual void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event);
		virtual void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event);
		virtual void _onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event);
		virtual void _onKeyPressedEvent(KeyPressedEvent &event);
		virtual void _onKeyReleasedEvent(KeyReleasedEvent &event);
		virtual void _onTextInputEvent(TextInputEvent &event);

	public:
		Widget(std::string name, Widget *parent);
		virtual ~Widget();

		void setZOrder(ZOrder zOrder);
		[[nodiscard]] ZOrder zOrder() const;
		[[nodiscard]] ZOrder absoluteZOrder() const;

		void setGeometry(const spk::Rect2D &geometry);
		void resize(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		[[nodiscard]] const ViewRegion &viewRegion() const;

		void dispatch(WindowResizedEvent &event);
		void dispatch(WindowMovedEvent &event);
		void dispatch(WindowFocusGainedEvent &event);
		void dispatch(WindowFocusLostEvent &event);
		void dispatch(MouseEnteredEvent &event);
		void dispatch(MouseLeftEvent &event);
		void dispatch(MouseMovedEvent &event);
		void dispatch(MouseWheelScrolledEvent &event);
		void dispatch(MouseButtonPressedEvent &event);
		void dispatch(MouseButtonReleasedEvent &event);
		void dispatch(MouseButtonDoubleClickedEvent &event);
		void dispatch(KeyPressedEvent &event);
		void dispatch(KeyReleasedEvent &event);
		void dispatch(TextInputEvent &event);

		void updateState(UpdateContext &context);
		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
	};
}
