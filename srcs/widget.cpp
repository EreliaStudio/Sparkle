#include "widget.hpp"

#include <cmath>
#include <utility>

#include "update_context.hpp"

#include "scissor_render_command.hpp"
#include "viewport_render_command.hpp"
#include "viewport_uniform_render_command.hpp"

namespace spk
{
	bool WidgetChildComparator::operator()(const Widget *lhs, const Widget *rhs) const
	{
		return lhs->zOrder() < rhs->zOrder();
	}

	Widget::Widget(std::string name, Widget *parent) :
		NameTrait(std::move(name)),
		_absoluteZOrder([this] {
			return _zOrder + (hasParent() ? this->parent()->absoluteZOrder() : 0);
		}),
		_viewRegion([this] {
			ViewRegion result{};
			spk::Rect2D absoluteGeometry = _geometry;
			if (hasParent())
			{
				const ViewRegion &parentRegion = this->parent()->viewRegion();
				absoluteGeometry.anchor += parentRegion.viewport.anchor;
				result.viewport = absoluteGeometry;
				result.scissor = absoluteGeometry.intersect(parentRegion.scissor);
			}
			else
			{
				result.viewport = result.scissor = absoluteGeometry;
			}
			return result;
		})
	{
		setParent(parent);
		_computeRatio();
		_onParentEditedContract = subscribeToParentEdition([this](const Widget *) {
			_computeRatio();
			_invalidateAbsoluteZOrder();
			_invalidateViewRegion();
		});
	}

	Widget::~Widget() = default;

	void Widget::_invalidateViewRegion()
	{
		_viewRegion.invalidate();
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->_invalidateViewRegion();
			}
		}
	}

	void Widget::_invalidateAbsoluteZOrder()
	{
		_absoluteZOrder.invalidate();
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->_invalidateAbsoluteZOrder();
			}
		}
	}

	void Widget::_computeRatio()
	{
		const spk::Vector2UInt referenceSize = hasParent() ? parent()->_geometry.size : _geometry.size;
		_anchorRatio.x = referenceSize.x != 0 ? static_cast<float>(_geometry.anchor.x) / static_cast<float>(referenceSize.x) : 0.0f;
		_anchorRatio.y = referenceSize.y != 0 ? static_cast<float>(_geometry.anchor.y) / static_cast<float>(referenceSize.y) : 0.0f;
		_sizeRatio.x = referenceSize.x != 0 ? static_cast<float>(_geometry.size.x) / static_cast<float>(referenceSize.x) : 1.0f;
		_sizeRatio.y = referenceSize.y != 0 ? static_cast<float>(_geometry.size.y) / static_cast<float>(referenceSize.y) : 1.0f;
	}

	spk::Rect2D Widget::_geometryFromRatio(const Widget &child) const
	{
		const float width = static_cast<float>(_geometry.size.x);
		const float height = static_cast<float>(_geometry.size.y);
		return spk::Rect2D{
			.anchor = spk::Vector2Int(static_cast<int>(std::lround(width * child._anchorRatio.x)), static_cast<int>(std::lround(height * child._anchorRatio.y))),
			.size = spk::Vector2UInt(static_cast<unsigned int>(std::lround(width * child._sizeRatio.x)), static_cast<unsigned int>(std::lround(height * child._sizeRatio.y)))};
	}

	void Widget::_resize(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_viewRegion.invalidate();
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->_resize(_geometryFromRatio(*child));
			}
		}
		_onGeometryChange();
	}

	template <typename TEvent>
	void Widget::_propagate(TEvent &event, void (Widget::*handler)(TEvent &))
	{
		if (!isActive() || event.consumed)
		{
			return;
		}
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->_propagate(event, handler);
			}
			if (event.consumed)
			{
				return;
			}
		}
		(this->*handler)(event);
	}

	void Widget::setZOrder(ZOrder zOrder)
	{
		if (_zOrder == zOrder)
		{
			return;
		}
		_zOrder = zOrder;
		_invalidateAbsoluteZOrder();
		notifyOrderingChange();
	}

	Widget::ZOrder Widget::zOrder() const
	{
		return _zOrder;
	}
	Widget::ZOrder Widget::absoluteZOrder() const
	{
		return _absoluteZOrder.get();
	}

	void Widget::setGeometry(const spk::Rect2D &geometry)
	{
		if (_geometry == geometry)
		{
			return;
		}
		_geometry = geometry;
		_computeRatio();
		_invalidateViewRegion();
		_onGeometryChange();
	}

	void Widget::resize(const spk::Rect2D &geometry)
	{
		if (_geometry != geometry)
		{
			_resize(geometry);
		}
	}

	const spk::Rect2D &Widget::geometry() const noexcept
	{
		return _geometry;
	}
	const ViewRegion &Widget::viewRegion() const
	{
		return _viewRegion.get();
	}

	void Widget::dispatch(WindowResizedEvent &event)
	{
		_propagate(event, &Widget::_onWindowResizedEvent);
	}
	void Widget::dispatch(WindowMovedEvent &event)
	{
		_propagate(event, &Widget::_onWindowMovedEvent);
	}
	void Widget::dispatch(WindowFocusGainedEvent &event)
	{
		_propagate(event, &Widget::_onWindowFocusGainedEvent);
	}
	void Widget::dispatch(WindowFocusLostEvent &event)
	{
		_propagate(event, &Widget::_onWindowFocusLostEvent);
	}
	void Widget::dispatch(MouseEnteredEvent &event)
	{
		_propagate(event, &Widget::_onMouseEnteredEvent);
	}
	void Widget::dispatch(MouseLeftEvent &event)
	{
		_propagate(event, &Widget::_onMouseLeftEvent);
	}
	void Widget::dispatch(MouseMovedEvent &event)
	{
		_propagate(event, &Widget::_onMouseMovedEvent);
	}
	void Widget::dispatch(MouseWheelScrolledEvent &event)
	{
		_propagate(event, &Widget::_onMouseWheelScrolledEvent);
	}
	void Widget::dispatch(MouseButtonPressedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonPressedEvent);
	}
	void Widget::dispatch(MouseButtonReleasedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonReleasedEvent);
	}
	void Widget::dispatch(MouseButtonDoubleClickedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonDoubleClickedEvent);
	}
	void Widget::dispatch(KeyPressedEvent &event)
	{
		_propagate(event, &Widget::_onKeyPressedEvent);
	}
	void Widget::dispatch(KeyReleasedEvent &event)
	{
		_propagate(event, &Widget::_onKeyReleasedEvent);
	}
	void Widget::dispatch(TextInputEvent &event)
	{
		_propagate(event, &Widget::_onTextInputEvent);
	}

	void Widget::updateState(UpdateContext &context)
	{
		if (!isActive())
		{
			return;
		}

		_updateState(context);

		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->updateState(context);
			}
		}
	}

	void Widget::_buildViewRegionCommands(spk::RenderSnapshot::Builder &builder)
	{
		auto &pass = builder.renderPass(Widget::OverlayKey);

		pass.emplace<spk::ViewportRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ViewportUniformRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ScissorRenderCommand>(_viewRegion->scissor);
	}

	void Widget::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (!isActive())
		{
			return;
		}

		_buildViewRegionCommands(builder);
		_buildRenderSnapshot(builder);

		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->buildRenderSnapshot(builder);
			}
		}
	}

	void Widget::_updateState(UpdateContext &)
	{
	}
	void Widget::_buildRenderSnapshot(spk::RenderSnapshot::Builder &)
	{
	}
	void Widget::_onGeometryChange()
	{
	}
	void Widget::_onWindowResizedEvent(WindowResizedEvent &)
	{
	}
	void Widget::_onWindowMovedEvent(WindowMovedEvent &)
	{
	}
	void Widget::_onWindowFocusGainedEvent(WindowFocusGainedEvent &)
	{
	}
	void Widget::_onWindowFocusLostEvent(WindowFocusLostEvent &)
	{
	}
	void Widget::_onMouseEnteredEvent(MouseEnteredEvent &)
	{
	}
	void Widget::_onMouseLeftEvent(MouseLeftEvent &)
	{
	}
	void Widget::_onMouseMovedEvent(MouseMovedEvent &)
	{
	}
	void Widget::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &)
	{
	}
	void Widget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
	}
	void Widget::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &)
	{
	}
	void Widget::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &)
	{
	}
	void Widget::_onKeyPressedEvent(KeyPressedEvent &)
	{
	}
	void Widget::_onKeyReleasedEvent(KeyReleasedEvent &)
	{
	}
	void Widget::_onTextInputEvent(TextInputEvent &)
	{
	}
}
