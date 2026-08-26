#include "ui/layout/layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "ui/widget.hpp"

namespace
{
	constexpr float Epsilon = 0.001f;

	float _sum(const std::vector<float> &values)
	{
		float result = 0.0f;
		for (float value : values)
		{
			result += value;
		}
		return result;
	}

	template <typename THints, typename TComponent>
	void _shrink(std::vector<float> &sizes, const THints &hints, float amount, TComponent component)
	{
		while (amount > Epsilon)
		{
			std::size_t count = 0;
			for (std::size_t i = 0; i < sizes.size(); ++i)
			{
				count += (sizes[i] > component(hints[i].minimal) + Epsilon);
			}
			if (count == 0)
			{
				return;
			}

			const float share = amount / static_cast<float>(count);
			float consumed = 0.0f;
			for (std::size_t i = 0; i < sizes.size(); ++i)
			{
				const float capacity = sizes[i] - component(hints[i].minimal);
				const float delta = std::min(share, std::max(0.0f, capacity));
				sizes[i] -= delta;
				consumed += delta;
			}
			if (consumed <= Epsilon)
			{
				return;
			}
			amount -= consumed;
		}
	}

	template <typename THints, typename TComponent>
	void _grow(std::vector<float> &sizes, const THints &hints, float amount, TComponent component)
	{
		while (amount > Epsilon)
		{
			std::size_t count = 0;
			for (std::size_t i = 0; i < sizes.size(); ++i)
			{
				count += (sizes[i] + Epsilon < component(hints[i].maximal));
			}
			if (count == 0)
			{
				return;
			}

			const float share = amount / static_cast<float>(count);
			float consumed = 0.0f;
			for (std::size_t i = 0; i < sizes.size(); ++i)
			{
				const float capacity = component(hints[i].maximal) - sizes[i];
				const float delta = std::min(share, std::max(0.0f, capacity));
				sizes[i] += delta;
				consumed += delta;
			}
			if (consumed <= Epsilon)
			{
				return;
			}
			amount -= consumed;
		}
	}

	uint32_t _toDimension(float value) noexcept
	{
		if (value <= 0.0f)
		{
			return 0;
		}
		const float maximum = static_cast<float>(std::numeric_limits<uint32_t>::max());
		return static_cast<uint32_t>(std::lround(std::min(value, maximum)));
	}

	uint32_t _clampedDimension(uint32_t available, float minimal, float maximal)
	{
		if (static_cast<float>(available) < minimal)
		{
			return available;
		}
		const float value = std::clamp(static_cast<float>(available), minimal, maximal);
		return _toDimension(value);
	}

	void _applyPolicy(float &minimal, float &maximal, float &preferred, spk::Layout::SizePolicy policy)
	{
		if (policy == spk::Layout::SizePolicy::Fixed)
		{
			minimal = maximal = preferred;
		}
		else if (policy == spk::Layout::SizePolicy::Minimum)
		{
			preferred = maximal = minimal;
		}
	}

	int32_t _horizontalOffset(spk::Alignment::Horizontal alignment, uint32_t available, uint32_t used)
	{
		if (used >= available)
		{
			return 0;
		}
		const uint32_t freeSpace = available - used;
		if (alignment == spk::Alignment::Horizontal::Center)
		{
			return static_cast<int32_t>(freeSpace / 2);
		}
		if (alignment == spk::Alignment::Horizontal::Right)
		{
			return static_cast<int32_t>(freeSpace);
		}
		return 0;
	}

	int32_t _verticalOffset(spk::Alignment::Vertical alignment, uint32_t available, uint32_t used)
	{
		if (used >= available)
		{
			return 0;
		}
		const uint32_t freeSpace = available - used;
		if (alignment == spk::Alignment::Vertical::Center)
		{
			return static_cast<int32_t>(freeSpace / 2);
		}
		if (alignment == spk::Alignment::Vertical::Bottom)
		{
			return static_cast<int32_t>(freeSpace);
		}
		return 0;
	}
}

namespace spk
{
	Layout::Element::Element(Layout &owner, Widget *widget, SizeSettings sizeSettings) :
		_owner(&owner),
		_widget(widget),
		_resizeable(widget),
		_sizeSettings(sizeSettings),
		_sizeHintEditionContract(widget->subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_owner->updateSizeHint();
		}))
	{
	}

	Layout::Element::Element(Layout &owner, Layout *layout, SizeSettings sizeSettings) :
		_owner(&owner),
		_layout(layout),
		_resizeable(layout),
		_sizeSettings(sizeSettings),
		_sizeHintEditionContract(layout->subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_owner->updateSizeHint();
		}))
	{
	}

	Widget *Layout::Element::widget() const noexcept
	{
		return _widget;
	}

	Layout *Layout::Element::layout() const noexcept
	{
		return _layout;
	}

	bool Layout::Element::isWidget() const noexcept
	{
		return _widget != nullptr;
	}

	bool Layout::Element::isLayout() const noexcept
	{
		return _layout != nullptr;
	}

	ResizeableTrait::SizeHint Layout::Element::sizeHint() const
	{
		SizeHint result = _resizeable->sizeHint();
		_applyPolicy(result.minimal.x, result.maximal.x, result.preferred.x, _sizeSettings.horizontal);
		_applyPolicy(result.minimal.y, result.maximal.y, result.preferred.y, _sizeSettings.vertical);
		return result;
	}

	Vector2 Layout::Element::minimalSize() const
	{
		return sizeHint().minimal;
	}

	Vector2 Layout::Element::maximalSize() const
	{
		return sizeHint().maximal;
	}

	Vector2 Layout::Element::preferredSize() const
	{
		return sizeHint().preferred;
	}

	void Layout::Element::setSizeSettings(SizeSettings sizeSettings)
	{
		if (_sizeSettings == sizeSettings)
		{
			return;
		}
		_sizeSettings = sizeSettings;
		_owner->updateSizeHint();
	}

	const Layout::SizeSettings &Layout::Element::sizeSettings() const noexcept
	{
		return _sizeSettings;
	}

	void Layout::Element::setHorizontalAlignment(Alignment::Horizontal alignment)
	{
		_alignment.horizontal = alignment;
	}

	Alignment::Horizontal Layout::Element::horizontalAlignment() const noexcept
	{
		return _alignment.horizontal;
	}

	void Layout::Element::setVerticalAlignment(Alignment::Vertical alignment)
	{
		_alignment.vertical = alignment;
	}

	Alignment::Vertical Layout::Element::verticalAlignment() const noexcept
	{
		return _alignment.vertical;
	}

	void Layout::Element::setAlignment(Alignment alignment)
	{
		_alignment = alignment;
	}

	Alignment Layout::Element::alignment() const noexcept
	{
		return _alignment;
	}

	void Layout::Element::setGeometry(const Rect2D &cell) const
	{
		const SizeHint hint = sizeHint();
		const uint32_t width = _clampedDimension(cell.width, hint.minimal.x, hint.maximal.x);
		const uint32_t height = _clampedDimension(cell.height, hint.minimal.y, hint.maximal.y);
		const int32_t x = cell.x + _horizontalOffset(_alignment.horizontal, cell.width, width);
		const int32_t y = cell.y + _verticalOffset(_alignment.vertical, cell.height, height);
		const Rect2D geometry = Layout::_rect(x, y, width, height);
		if (_widget != nullptr)
		{
			_widget->setGeometry(geometry);
		}
		else
		{
			_layout->setGeometry(geometry);
		}
	}

	std::unique_ptr<Layout::Element> Layout::_createElement(Widget *widget, SizeSettings sizeSettings)
	{
		if (widget == nullptr)
		{
			throw std::invalid_argument("Layout cannot hold a null widget");
		}
		return std::unique_ptr<Element>(new Element(*this, widget, sizeSettings));
	}

	std::unique_ptr<Layout::Element> Layout::_createElement(Layout *layout, SizeSettings sizeSettings)
	{
		if (layout == nullptr)
		{
			throw std::invalid_argument("Layout cannot hold a null layout");
		}
		if (layout == this)
		{
			throw std::invalid_argument("Layout cannot contain itself");
		}
		return std::unique_ptr<Element>(new Element(*this, layout, sizeSettings));
	}

	void Layout::_eraseElement(Element *element)
	{
		const auto it = std::find_if(_elements.begin(), _elements.end(), [element](const auto &candidate) {
			return candidate.get() == element;
		});
		if (it == _elements.end())
		{
			return;
		}
		_elements.erase(it);
		updateSizeHint();
	}

	std::vector<float> Layout::_resolveAxis(const std::vector<SizeHint> &hints, float availableSize, bool horizontal)
	{
		const auto component = [horizontal](const Vector2 &value) {
			return horizontal ? value.x : value.y;
		};
		std::vector<float> result;
		result.reserve(hints.size());
		for (const SizeHint &hint : hints)
		{
			result.push_back(component(hint.preferred));
		}
		const float difference = std::max(0.0f, availableSize) - _sum(result);
		if (difference < 0.0f)
		{
			_shrink(result, hints, -difference, component);
			const float resolvedTotal = _sum(result);
			const float clampedAvailable = std::max(0.0f, availableSize);
			if (resolvedTotal > clampedAvailable + Epsilon && resolvedTotal > Epsilon)
			{
				const float scale = clampedAvailable / resolvedTotal;
				for (float &size : result)
				{
					size *= scale;
				}
			}
		}
		else
		{
			_grow(result, hints, difference, component);
		}
		return result;
	}

	uint32_t Layout::_dimension(float value) noexcept
	{
		return _toDimension(value);
	}

	Rect2D Layout::_rect(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept
	{
		Rect2D result;
		result.anchor = {x, y};
		result.size = {width, height};
		return result;
	}

	void Layout::_setComputedSizeHint(const SizeHint &hint)
	{
		setSizeHint(hint);
	}

	void Layout::updateSizeHint()
	{
		_updateSizeHint();
	}

	void Layout::setGeometry(const Rect2D &geometry)
	{
		_applyGeometry(geometry);
	}

	void Layout::clear()
	{
		_elements.clear();
		updateSizeHint();
	}

	void Layout::setElementPadding(const Vector2UInt &padding)
	{
		if (_elementPadding == padding)
		{
			return;
		}
		_elementPadding = padding;
		updateSizeHint();
	}

	const Vector2UInt &Layout::elementPadding() const noexcept
	{
		return _elementPadding;
	}

	const std::vector<std::unique_ptr<Layout::Element>> &Layout::elements() const noexcept
	{
		return _elements;
	}
}
