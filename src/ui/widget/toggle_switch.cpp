#include "ui/widget/toggle_switch.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "core/context/update_context.hpp"
#include "graphics/sprite_sheet.hpp"
#include "rendering/command/image_render_command.hpp"
#include "rendering/command/nine_slice_render_command.hpp"
#include "rendering/command/scissor_render_command.hpp"

namespace spk
{
	ToggleSwitch::ToggleSwitch(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_outline(this->name() + ".outline", this),
		_thumb(this->name() + ".thumb", this)
	{
		applyStyle(defaultStyle);
		_outline.setZOrder(1.0f);
		_thumb.setZOrder(2.0f);
		_updateSizeHint();
		activate();
	}

	void ToggleSwitch::applyStyle(const Style &style)
	{
		if (style.toggleSwitchOffBackground != nullptr)
		{
			setUncheckedBackgroundTexture(style.toggleSwitchOffBackground.get());
		}
		if (style.toggleSwitchOnBackground != nullptr)
		{
			setCheckedBackgroundTexture(style.toggleSwitchOnBackground.get());
		}
		if (style.toggleSwitchOutline != nullptr)
		{
			_outline.setSpriteSheet(style.toggleSwitchOutline.get());
		}
		if (style.toggleSwitchThumb != nullptr)
		{
			_thumb.setSpriteSheet(style.toggleSwitchThumb.get());
		}
	}

	void ToggleSwitch::_updateVisualGeometry()
	{
		_outline.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
		const unsigned int left = std::min(_padding.x, geometry().width / 2);
		const unsigned int top = std::min(_padding.y, geometry().height / 2);
		const unsigned int availableWidth = geometry().width - 2 * left;
		const unsigned int availableHeight = geometry().height - 2 * top;
		const Vector2UInt size{std::min(_thumbSize.x, availableWidth), std::min(_thumbSize.y, availableHeight)};
		Rect2D thumb{Vector2Int{static_cast<int>(left), static_cast<int>(top)}, size};
		if (_orientation == Orientation::Horizontal)
		{
			const unsigned int travel = availableWidth > size.x ? availableWidth - size.x : 0;
			thumb.x += static_cast<int>(std::lround(static_cast<float>(travel) * _visualRatio));
			thumb.y += static_cast<int>((availableHeight - size.y) / 2);
		}
		else
		{
			const unsigned int travel = availableHeight > size.y ? availableHeight - size.y : 0;
			thumb.x += static_cast<int>((availableWidth - size.x) / 2);
			thumb.y += static_cast<int>(std::lround(static_cast<float>(travel) * (1.0f - _visualRatio)));
		}
		_thumb.setGeometry(thumb);
	}

	void ToggleSwitch::_updateSizeHint()
	{
		Vector2 preferred;
		if (_orientation == Orientation::Horizontal)
		{
			preferred = {static_cast<float>(2 * _thumbSize.x + 2 * _padding.x), static_cast<float>(_thumbSize.y + 2 * _padding.y)};
		}
		else
		{
			preferred = {static_cast<float>(_thumbSize.x + 2 * _padding.x), static_cast<float>(2 * _thumbSize.y + 2 * _padding.y)};
		}
		setSizeHint({preferred, {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}, preferred});
	}

	void ToggleSwitch::_updateState(UpdateContext &context)
	{
		if (!_animating)
		{
			return;
		}
		_animationElapsed += context.deltaTime;
		const double total = std::chrono::duration<double>(_animationDuration).count();
		const float t = total <= 0.0 ? 1.0f : std::clamp(static_cast<float>(std::chrono::duration<double>(_animationElapsed).count() / total), 0.0f, 1.0f);
		const float target = _checked ? 1.0f : 0.0f;
		_visualRatio = _animationStart + (target - _animationStart) * t;
		if (t >= 1.0f)
		{
			_visualRatio = target;
			_animating = false;
		}
		_updateVisualGeometry();
	}

	void ToggleSwitch::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (geometry().width == 0 || geometry().height == 0)
		{
			return;
		}
		auto &pass = builder.renderPass(targetRenderPass());
		const Rect2D full{Vector2Int{0, 0}, geometry().size};
		const ViewRegion &region = viewRegion();
		Rect2D checkedClip = region.viewport;
		Rect2D uncheckedClip = region.viewport;
		if (_orientation == Orientation::Horizontal)
		{
			const unsigned int boundary = static_cast<unsigned int>(std::lround(static_cast<float>(geometry().width) * _visualRatio));
			checkedClip.width = boundary;
			uncheckedClip.x += static_cast<int>(boundary);
			uncheckedClip.width -= boundary;
		}
		else
		{
			const unsigned int checkedHeight = static_cast<unsigned int>(std::lround(static_cast<float>(geometry().height) * _visualRatio));
			checkedClip.y += static_cast<int>(geometry().height - checkedHeight);
			checkedClip.height = checkedHeight;
			uncheckedClip.height -= checkedHeight;
		}
		auto renderBackground = [&pass, &full](
									const Texture *texture,
									const Texture::Section &section,
									const SpriteSheet *nineSlice,
									const Rect2D &clip,
									const Rect2D &scissor) {
			if (texture == nullptr || clip.width == 0 || clip.height == 0)
			{
				return;
			}
			pass.emplace<ScissorRenderCommand>(clip.intersect(scissor));
			if (nineSlice != nullptr)
			{
				const Vector2UInt cornerSize{
					std::min(nineSlice->size().x / 3, full.width / 2),
					std::min(nineSlice->size().y / 3, full.height / 2)};
				pass.emplace<NineSliceRenderCommand>(nineSlice, full, cornerSize, 0.0f);
			}
			else
			{
				pass.emplace<ImageRenderCommand>(texture, section, full, 0.0f);
			}
		};
		if (_checkedTexture != nullptr && checkedClip.width != 0 && checkedClip.height != 0)
		{
			renderBackground(_checkedTexture, _checkedSection, _checkedNineSlice, checkedClip, region.scissor);
		}
		if (_uncheckedTexture != nullptr && uncheckedClip.width != 0 && uncheckedClip.height != 0)
		{
			renderBackground(_uncheckedTexture, _uncheckedSection, _uncheckedNineSlice, uncheckedClip, region.scissor);
		}
		pass.emplace<ScissorRenderCommand>(region.scissor);
	}

	void ToggleSwitch::_onGeometryChange()
	{
		_updateVisualGeometry();
	}
	void ToggleSwitch::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (event.record.button == Mouse::Button::Left && viewRegion().viewport.contains(event.device.position))
		{
			_pressed = true;
			event.consumed = true;
		}
	}
	void ToggleSwitch::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !_pressed)
		{
			return;
		}
		_pressed = false;
		if (viewRegion().viewport.contains(event.device.position))
		{
			toggle();
			event.consumed = true;
		}
	}
	void ToggleSwitch::setChecked(bool checked)
	{
		if (_checked == checked)
		{
			return;
		}
		_checked = checked;
		_animationStart = _visualRatio;
		_animationElapsed = {};
		_animating = _animationDuration > Duration::zero();
		if (!_animating)
		{
			_visualRatio = checked ? 1.0f : 0.0f;
			_updateVisualGeometry();
		}
		_stateProvider.trigger(_checked);
	}
	void ToggleSwitch::toggle()
	{
		setChecked(!_checked);
	}
	void ToggleSwitch::setOrientation(Orientation orientation)
	{
		if (_orientation != orientation)
		{
			_orientation = orientation;
			_updateSizeHint();
			_updateVisualGeometry();
		}
	}
	void ToggleSwitch::setPadding(const Vector2UInt &padding)
	{
		_padding = padding;
		_updateSizeHint();
		_updateVisualGeometry();
	}
	void ToggleSwitch::setThumbSize(const Vector2UInt &size)
	{
		_thumbSize = size;
		_updateSizeHint();
		_updateVisualGeometry();
	}
	void ToggleSwitch::setAnimationDuration(Duration duration)
	{
		if (duration < Duration::zero())
		{
			throw std::invalid_argument("ToggleSwitch animation duration cannot be negative");
		}
		_animationDuration = duration;
		if (duration == Duration::zero() && _animating)
		{
			_animating = false;
			_visualRatio = _checked ? 1.0f : 0.0f;
			_updateVisualGeometry();
		}
	}
	void ToggleSwitch::setUncheckedBackgroundTexture(const Texture *texture, const Texture::Section &section)
	{
		if (texture == nullptr)
		{
			throw std::invalid_argument("ToggleSwitch unchecked texture cannot be null");
		}
		_uncheckedTexture = texture;
		_uncheckedSection = section;
		_uncheckedNineSlice = nullptr;
	}
	void ToggleSwitch::setUncheckedBackgroundTexture(const SpriteSheet *texture)
	{
		if (texture == nullptr)
		{
			throw std::invalid_argument("ToggleSwitch unchecked texture cannot be null");
		}
		if (texture->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("ToggleSwitch unchecked nine-slice texture must be a 3x3 sprite sheet");
		}
		_uncheckedTexture = texture;
		_uncheckedSection = Texture::Section::whole;
		_uncheckedNineSlice = texture;
	}
	void ToggleSwitch::setCheckedBackgroundTexture(const Texture *texture, const Texture::Section &section)
	{
		if (texture == nullptr)
		{
			throw std::invalid_argument("ToggleSwitch checked texture cannot be null");
		}
		_checkedTexture = texture;
		_checkedSection = section;
		_checkedNineSlice = nullptr;
	}
	void ToggleSwitch::setCheckedBackgroundTexture(const SpriteSheet *texture)
	{
		if (texture == nullptr)
		{
			throw std::invalid_argument("ToggleSwitch checked texture cannot be null");
		}
		if (texture->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("ToggleSwitch checked nine-slice texture must be a 3x3 sprite sheet");
		}
		_checkedTexture = texture;
		_checkedSection = Texture::Section::whole;
		_checkedNineSlice = texture;
	}
	void ToggleSwitch::setOutlineTexture(const SpriteSheet *texture)
	{
		_outline.setSpriteSheet(texture);
	}
	void ToggleSwitch::setOutlineCornerSize(const Vector2Int &size)
	{
		_outline.setCornerSize(size);
	}
	void ToggleSwitch::setThumbTexture(const SpriteSheet *texture)
	{
		_thumb.setSpriteSheet(texture);
	}
	void ToggleSwitch::setThumbCornerSize(const Vector2Int &size)
	{
		_thumb.setCornerSize(size);
	}
	bool ToggleSwitch::isChecked() const noexcept
	{
		return _checked;
	}
	bool ToggleSwitch::isAnimating() const noexcept
	{
		return _animating;
	}
	float ToggleSwitch::visualRatio() const noexcept
	{
		return _visualRatio;
	}
	Orientation ToggleSwitch::orientation() const noexcept
	{
		return _orientation;
	}
	const Vector2UInt &ToggleSwitch::padding() const noexcept
	{
		return _padding;
	}
	const Vector2UInt &ToggleSwitch::thumbSize() const noexcept
	{
		return _thumbSize;
	}
	ToggleSwitch::Duration ToggleSwitch::animationDuration() const noexcept
	{
		return _animationDuration;
	}
	const Texture *ToggleSwitch::uncheckedBackgroundTexture() const noexcept
	{
		return _uncheckedTexture;
	}
	const Texture::Section &ToggleSwitch::uncheckedBackgroundSection() const noexcept
	{
		return _uncheckedSection;
	}
	const Texture *ToggleSwitch::checkedBackgroundTexture() const noexcept
	{
		return _checkedTexture;
	}
	const Texture::Section &ToggleSwitch::checkedBackgroundSection() const noexcept
	{
		return _checkedSection;
	}
	const SpriteSheet *ToggleSwitch::outlineTexture() const noexcept
	{
		return _outline.spriteSheet();
	}
	const Vector2Int &ToggleSwitch::outlineCornerSize() const noexcept
	{
		return _outline.cornerSize();
	}
	const SpriteSheet *ToggleSwitch::thumbTexture() const noexcept
	{
		return _thumb.spriteSheet();
	}
	const Vector2Int &ToggleSwitch::thumbCornerSize() const noexcept
	{
		return _thumb.cornerSize();
	}
	ToggleSwitch::StateContract ToggleSwitch::subscribeToState(StateCallback callback)
	{
		return _stateProvider.subscribe(std::move(callback));
	}
}
