#include "ui/widget.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include "core/context/update_context.hpp"

#include "exception.hpp"
#include "graphics/font.hpp"
#include "graphics/image.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/sprite_sheet.hpp"
#include "rendering/command/scissor_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Widget::Style::Style() = default;
	Widget::Style::~Style() = default;
	Widget::Style::Style(Style &&) noexcept = default;
	Widget::Style &Widget::Style::operator=(Style &&) noexcept = default;

	CachedData<Widget::Style> Widget::defaultStyle([] {
		const auto bytes = [](std::string_view path) {
			const resources::Data data = resources::get(path);
			return std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(data.data()), data.size()};
		};
		const auto font = [](std::string_view path) {
			const resources::Data data = resources::get(path);
			Font::Data fontData(data.size());
			std::memcpy(fontData.data(), data.data(), data.size());
			return std::make_unique<Font>(Font::fromRawData(std::move(fontData)));
		};

		Style style;
		style.font = font("fonts/arial.ttf");
		style.iconsetImage = std::make_unique<Image>(bytes("textures/default_iconset.png"));
		style.iconset = std::make_unique<SpriteSheet>(bytes("textures/default_iconset.png"), Vector2UInt{10, 10});
		style.nineSlice = std::make_unique<SpriteSheet>(bytes("textures/default_nine_slice.png"), Vector2UInt{3, 3});
		style.darkNineSlice = std::make_unique<SpriteSheet>(bytes("textures/default_nine_slice_dark.png"), Vector2UInt{3, 3});
		style.darkerNineSlice = std::make_unique<SpriteSheet>(bytes("textures/default_nine_slice_darker.png"), Vector2UInt{3, 3});
		style.lightNineSlice = std::make_unique<SpriteSheet>(bytes("textures/default_nine_slice_light.png"), Vector2UInt{3, 3});
		style.sliderBody = std::make_unique<SpriteSheet>(bytes("textures/default_slider_body.png"), Vector2UInt{3, 3});
		style.menuBreak = std::make_unique<SpriteSheet>(bytes("textures/default_break.png"), Vector2UInt{3, 1});
		style.toggleSwitchOutline = std::make_unique<SpriteSheet>(bytes("textures/default_toggle_switch_nine_slice.png"), Vector2UInt{3, 3});
		style.toggleSwitchThumb = std::make_unique<SpriteSheet>(bytes("textures/default_toggle_switch_thumb_nine_slice.png"), Vector2UInt{3, 3});
		style.toggleSwitchOffBackground = std::make_unique<SpriteSheet>(bytes("textures/default_toggle_switch_off_background.png"), Vector2UInt{3, 3});
		style.toggleSwitchOnBackground = std::make_unique<SpriteSheet>(bytes("textures/default_toggle_switch_on_background.png"), Vector2UInt{3, 3});
		return style;
	});

	void Widget::applyStyle(const Style &)
	{
	}

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
		const Style &style = defaultStyle;
		(void)style;
		_initialActivationContract = subscribeToActivation([this]() {
			_acceptChildSizeHintEditions = true;
		});
		setParent(parent);
		_computeRatio();
		_onParentEditedContract = subscribeToParentEdition([this](const Widget *) {
			_computeRatio();
			_invalidateAbsoluteZOrder();
			_invalidateViewRegion();
		});
	}

	Widget::~Widget()
	{
		while (!children().empty())
		{
			children().back()->setParent(nullptr);
		}

		setParent(nullptr);
	}

	void Widget::_onChildAdded(Widget *child)
	{
		auto contract = child->subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_onChildSizeHintEdition();
		});
		_childSizeHintEditionContracts.emplace(child, std::move(contract));
	}

	void Widget::_onChildRemoved(Widget *child)
	{
		_childSizeHintEditionContracts.erase(child);
	}

	void Widget::_onChildSizeHintEdition()
	{
		if (!_acceptChildSizeHintEditions)
		{
			return;
		}

		const ResizeableTrait::SizeHint previousSizeHint = sizeHint();
		_updateSizeHint();

		if (sizeHint() == previousSizeHint || !hasParent())
		{
			_onGeometryChange();
		}
	}

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
	void Widget::_propagate(TEvent &event, void (Widget::*handler)(TEvent &), std::string_view eventName)
	{
		if (!isActive() || event.consumed)
		{
			return;
		}
		try
		{
			constexpr bool PointerEvent =
				std::is_same_v<TEvent, MouseEnteredEvent> ||
				std::is_same_v<TEvent, MouseLeftEvent> ||
				std::is_same_v<TEvent, MouseMovedEvent> ||
				std::is_same_v<TEvent, MouseWheelScrolledEvent> ||
				std::is_same_v<TEvent, MouseButtonPressedEvent> ||
				std::is_same_v<TEvent, MouseButtonReleasedEvent> ||
				std::is_same_v<TEvent, MouseButtonDoubleClickedEvent>;

			if constexpr (PointerEvent)
			{
				for (auto it = children().rbegin(); it != children().rend(); ++it)
				{
					if (*it != nullptr)
					{
						(*it)->_propagate(event, handler, eventName);
					}
					if (event.consumed)
					{
						return;
					}
				}
			}
			else
			{
				for (Widget *child : children())
				{
					if (child != nullptr)
					{
						child->_propagate(event, handler, eventName);
					}
					if (event.consumed)
					{
						return;
					}
				}
			}
			(this->*handler)(event);
		} catch (spk::Exception &exception)
		{
			exception.addContext(
				"Exception while dispatching " + std::string(eventName) + " to widget [" + name() + "]");
			throw;
		} catch (...)
		{
			throw spk::Exception(
				"Exception while dispatching " + std::string(eventName) + " to widget [" + name() + "]",
				std::current_exception());
		}
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

	Widget &Widget::root() noexcept
	{
		Widget *result = this;
		while (result->hasParent())
		{
			result = result->parent();
		}
		return *result;
	}

	const Widget &Widget::root() const noexcept
	{
		const Widget *result = this;
		while (result->hasParent())
		{
			result = result->parent();
		}
		return *result;
	}

	void Widget::setTargetRenderPass(const RenderPass::Key &key)
	{
		_targetRenderPassOverride = key;
	}

	void Widget::inheritTargetRenderPass()
	{
		_targetRenderPassOverride.reset();
	}

	bool Widget::hasTargetRenderPassOverride() const noexcept
	{
		return _targetRenderPassOverride.has_value();
	}

	const RenderPass::Key &Widget::targetRenderPass() const noexcept
	{
		if (_targetRenderPassOverride.has_value())
		{
			return *_targetRenderPassOverride;
		}
		return hasParent() ? parent()->targetRenderPass() : OverlayKey;
	}

	void Widget::dispatch(WindowResizedEvent &event)
	{
		_propagate(event, &Widget::_onWindowResizedEvent, "window resized event");
	}
	void Widget::dispatch(WindowMovedEvent &event)
	{
		_propagate(event, &Widget::_onWindowMovedEvent, "window moved event");
	}
	void Widget::dispatch(WindowFocusGainedEvent &event)
	{
		_propagate(event, &Widget::_onWindowFocusGainedEvent, "window focus gained event");
	}
	void Widget::dispatch(WindowFocusLostEvent &event)
	{
		_propagate(event, &Widget::_onWindowFocusLostEvent, "window focus lost event");
	}
	void Widget::dispatch(MouseEnteredEvent &event)
	{
		_propagate(event, &Widget::_onMouseEnteredEvent, "mouse entered event");
	}
	void Widget::dispatch(MouseLeftEvent &event)
	{
		_propagate(event, &Widget::_onMouseLeftEvent, "mouse left event");
	}
	void Widget::dispatch(MouseMovedEvent &event)
	{
		_propagate(event, &Widget::_onMouseMovedEvent, "mouse moved event");
	}
	void Widget::dispatch(MouseWheelScrolledEvent &event)
	{
		_propagate(event, &Widget::_onMouseWheelScrolledEvent, "mouse wheel scrolled event");
	}
	void Widget::dispatch(MouseButtonPressedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonPressedEvent, "mouse button pressed event");
	}
	void Widget::dispatch(MouseButtonReleasedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonReleasedEvent, "mouse button released event");
	}
	void Widget::dispatch(MouseButtonDoubleClickedEvent &event)
	{
		_propagate(event, &Widget::_onMouseButtonDoubleClickedEvent, "mouse button double-clicked event");
	}
	void Widget::dispatch(KeyPressedEvent &event)
	{
		_propagate(event, &Widget::_onKeyPressedEvent, "key pressed event");
	}
	void Widget::dispatch(KeyReleasedEvent &event)
	{
		_propagate(event, &Widget::_onKeyReleasedEvent, "key released event");
	}
	void Widget::dispatch(TextInputEvent &event)
	{
		_propagate(event, &Widget::_onTextInputEvent, "text input event");
	}

	void Widget::updateState(UpdateContext &context)
	{
		if (!isActive())
		{
			return;
		}

		try
		{
			_updateState(context);

			for (Widget *child : children())
			{
				if (child != nullptr)
				{
					child->updateState(context);
				}
			}
		} catch (spk::Exception &exception)
		{
			exception.addContext("Exception while updating widget [" + name() + "]");
			throw;
		} catch (...)
		{
			throw spk::Exception(
				"Exception while updating widget [" + name() + "]",
				std::current_exception());
		}
	}

	void Widget::_buildViewRegionCommands(spk::RenderSnapshot::Builder &builder)
	{
		auto &pass = builder.renderPass(targetRenderPass());

		pass.emplace<spk::ViewportRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ViewportUniformRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ScissorRenderCommand>(_viewRegion->scissor);
	}

	void Widget::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (!isActive() ||
			viewRegion().scissor.height == 0 ||
			viewRegion().scissor.width == 0)
		{
			return;
		}

		try
		{
			_buildViewRegionCommands(builder);
			_buildRenderSnapshot(builder);

			for (Widget *child : children())
			{
				if (child != nullptr)
				{
					child->buildRenderSnapshot(builder);
				}
			}
		} catch (spk::Exception &exception)
		{
			exception.addContext("Exception while building render snapshot of widget [" + name() + "]");
			throw;
		} catch (...)
		{
			throw spk::Exception(
				"Exception while building render snapshot of widget [" + name() + "]",
				std::current_exception());
		}
	}

	void Widget::_updateState(UpdateContext &)
	{
	}
	void Widget::_buildRenderSnapshot(spk::RenderSnapshot::Builder &)
	{
	}
	void Widget::_updateSizeHint()
	{
	}
	void Widget::_onGeometryChange()
	{
	}

	void Widget::observePointer(MouseMovedEvent &event)
	{
		if (!isActive())
		{
			return;
		}
		_onPassiveMouseMovedEvent(event);
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->observePointer(event);
			}
		}
	}

	void Widget::observePointer(MouseButtonPressedEvent &event)
	{
		if (!isActive())
		{
			return;
		}
		_onPassiveMouseButtonPressedEvent(event);
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->observePointer(event);
			}
		}
	}

	void Widget::observeKeyboard(KeyPressedEvent &event)
	{
		if (!isActive())
		{
			return;
		}
		_onPassiveKeyPressedEvent(event);
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->observeKeyboard(event);
			}
		}
	}

	void Widget::observeKeyboard(KeyReleasedEvent &event)
	{
		if (!isActive())
		{
			return;
		}
		_onPassiveKeyReleasedEvent(event);
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->observeKeyboard(event);
			}
		}
	}
	void Widget::_onFocusAcquired(FocusMode::Channel) noexcept
	{
	}
	void Widget::_onFocusReleased(FocusMode::Channel) noexcept
	{
	}

	void Widget::notifyFocusAcquired(FocusMode::Channel channel) noexcept
	{
		_onFocusAcquired(channel);
	}

	void Widget::notifyFocusReleased(FocusMode::Channel channel) noexcept
	{
		_onFocusReleased(channel);
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
	void Widget::_onPassiveMouseMovedEvent(MouseMovedEvent &)
	{
	}
	void Widget::_onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &)
	{
	}
	void Widget::_onPassiveKeyPressedEvent(KeyPressedEvent &)
	{
	}
	void Widget::_onPassiveKeyReleasedEvent(KeyReleasedEvent &)
	{
	}
}
