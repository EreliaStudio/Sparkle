#include "ui/widget.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
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
			spk::Rect2D absoluteGeometry = geometry();
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
		_deactivationContract = subscribeToDeactivation([this]() {
			_onDeactivation();
		});
		if (parent != nullptr)
		{
			setParent(*parent);
		}
		_computeRatio();
		_onParentEditedContract = subscribeToParentEdition([this](const Widget *) {
			_computeRatio();
			_invalidateAbsoluteZOrder();
			_invalidateViewRegion();
		});
	}

	Widget::~Widget()
	{
		_destructionProvider.trigger(this);
		while (!children().empty())
		{
			children().back()->clearParent();
		}

		clearParent();
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

	bool Widget::_isAcceptingEvent() const
	{
		return isActive();
	}

	bool Widget::_canUpdate() const
	{
		return isActive();
	}

	bool Widget::_canBuildRenderSnapshot() const
	{
		return isActive() &&
			   viewRegion().scissor.height != 0 &&
			   viewRegion().scissor.width != 0;
	}

	void Widget::_propagateEvent(
		const std::function<void(EventDispatcher *)> &callback)
	{
		for (auto it = children().rbegin(); it != children().rend(); ++it)
		{
			if (*it != nullptr)
			{
				callback(*it);
			}
		}
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
		const spk::Vector2UInt referenceSize = hasParent() ? parent()->geometry().size : geometry().size;
		_anchorRatio.x = referenceSize.x != 0 ? static_cast<float>(geometry().anchor.x) / static_cast<float>(referenceSize.x) : 0.0f;
		_anchorRatio.y = referenceSize.y != 0 ? static_cast<float>(geometry().anchor.y) / static_cast<float>(referenceSize.y) : 0.0f;
		_sizeRatio.x = referenceSize.x != 0 ? static_cast<float>(geometry().size.x) / static_cast<float>(referenceSize.x) : 1.0f;
		_sizeRatio.y = referenceSize.y != 0 ? static_cast<float>(geometry().size.y) / static_cast<float>(referenceSize.y) : 1.0f;
	}

	spk::Rect2D Widget::_geometryFromRatio(const Widget &child) const
	{
		const float width = static_cast<float>(geometry().size.x);
		const float height = static_cast<float>(geometry().size.y);
		return spk::Rect2D{
			.anchor = spk::Vector2Int(static_cast<int>(std::lround(width * child._anchorRatio.x)), static_cast<int>(std::lround(height * child._anchorRatio.y))),
			.size = spk::Vector2UInt(static_cast<unsigned int>(std::lround(width * child._sizeRatio.x)), static_cast<unsigned int>(std::lround(height * child._sizeRatio.y)))};
	}

	void Widget::_resize(const spk::Rect2D &geometry)
	{
		setGeometry(geometry, false);
		_viewRegion.invalidate();
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->_resize(_geometryFromRatio(*child));
			}
		}
		notifyGeometryChange();
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

	void Widget::_onSetGeometry()
	{
		_computeRatio();
		_invalidateViewRegion();
	}

	void Widget::resize(const spk::Rect2D &geometry)
	{
		if (this->geometry() != geometry)
		{
			_resize(geometry);
		}
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

	void Widget::_afterUpdate(UpdateContext &context)
	{
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
		auto &pass = builder.renderPass(targetRenderPass());

		pass.emplace<spk::ViewportRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ViewportUniformRenderCommand>(_viewRegion->viewport);
		pass.emplace<spk::ScissorRenderCommand>(_viewRegion->scissor);
	}

	void Widget::_beforeBuildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		_buildViewRegionCommands(builder);
	}

	void Widget::_afterBuildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		for (Widget *child : children())
		{
			if (child != nullptr)
			{
				child->buildRenderSnapshot(builder);
			}
		}
	}

	void Widget::_onUpdateException(std::exception_ptr exception)
	{
		throw spk::Exception(
			"Exception while updating widget [" + name() + "]",
			std::move(exception));
	}

	void Widget::_onBuildRenderSnapshotException(std::exception_ptr exception)
	{
		throw spk::Exception(
			"Exception while building render snapshot of widget [" + name() + "]",
			std::move(exception));
	}

	void Widget::_onGeometryChange(const spk::Rect2D &)
	{
		_onGeometryChange();
	}
	void Widget::_updateSizeHint()
	{
	}
	void Widget::_onGeometryChange()
	{
	}

	void Widget::_onFocusAcquired(FocusMode::Channel) noexcept
	{
	}
	void Widget::_onFocusReleased(FocusMode::Channel) noexcept
	{
	}
	void Widget::_onDeactivation() noexcept
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

	Widget::DestructionContract Widget::subscribeToDestruction(DestructionCallback callback)
	{
		return _destructionProvider.subscribe(std::move(callback));
	}
}
