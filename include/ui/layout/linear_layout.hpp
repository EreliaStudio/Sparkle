#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "type/orientation.hpp"
#include "ui/layout/layout.hpp"

namespace spk
{
	template <Orientation TOrientation>
	class LinearLayout : public Layout
	{
	private:
		static constexpr bool Horizontal = TOrientation == Orientation::Horizontal;

		[[nodiscard]] static float _primary(const Vector2 &value) noexcept
		{
			if constexpr (Horizontal)
			{
				return value.x;
			}
			return value.y;
		}

		[[nodiscard]] static float _secondary(const Vector2 &value) noexcept
		{
			if constexpr (Horizontal)
			{
				return value.y;
			}
			return value.x;
		}

		[[nodiscard]] static uint32_t _primary(const Vector2UInt &value) noexcept
		{
			if constexpr (Horizontal)
			{
				return value.x;
			}
			return value.y;
		}

		[[nodiscard]] static uint32_t _secondary(const Vector2UInt &value) noexcept
		{
			if constexpr (Horizontal)
			{
				return value.y;
			}
			return value.x;
		}

		[[nodiscard]] std::vector<SizeHint> _primaryHints() const
		{
			std::vector<SizeHint> result;
			result.reserve(_elements.size());
			for (const auto &element : _elements)
			{
				result.push_back(element->sizeHint());
			}
			return result;
		}

		[[nodiscard]] float _paddingTotal() const noexcept
		{
			if (_elements.size() < 2)
			{
				return 0.0f;
			}
			return static_cast<float>(_elements.size() - 1) * static_cast<float>(_primary(_elementPadding));
		}

		[[nodiscard]] SizeHint _computedSizeHint() const
		{
			Vector2 minimal{};
			Vector2 preferred{};
			Vector2 maximal{};
			for (const auto &element : _elements)
			{
				_accumulateHint(element->sizeHint(), minimal, preferred, maximal);
			}
			const float padding = _paddingTotal();
			_addPrimary(minimal, padding);
			_addPrimary(preferred, padding);
			_addPrimary(maximal, padding);
			return {minimal, maximal, preferred};
		}

		static void _accumulateHint(const SizeHint &hint, Vector2 &minimal, Vector2 &preferred, Vector2 &maximal) noexcept
		{
			if constexpr (Horizontal)
			{
				minimal = {minimal.x + hint.minimal.x, std::max(minimal.y, hint.minimal.y)};
				preferred = {preferred.x + hint.preferred.x, std::max(preferred.y, hint.preferred.y)};
				maximal = {maximal.x + hint.maximal.x, std::max(maximal.y, hint.maximal.y)};
			}
			else
			{
				minimal = {std::max(minimal.x, hint.minimal.x), minimal.y + hint.minimal.y};
				preferred = {std::max(preferred.x, hint.preferred.x), preferred.y + hint.preferred.y};
				maximal = {std::max(maximal.x, hint.maximal.x), maximal.y + hint.maximal.y};
			}
		}

		static void _addPrimary(Vector2 &value, float amount) noexcept
		{
			if constexpr (Horizontal)
			{
				value.x += amount;
			}
			else
			{
				value.y += amount;
			}
		}

		[[nodiscard]] Rect2D _cell(const Rect2D &geometry, int32_t cursor, uint32_t primarySize) const noexcept
		{
			if constexpr (Horizontal)
			{
				return _rect(cursor, geometry.y, primarySize, geometry.height);
			}
			return _rect(geometry.x, cursor, geometry.width, primarySize);
		}

	protected:
		void _updateSizeHint() override
		{
			_setComputedSizeHint(_computedSizeHint());
		}

		void _applyGeometry(const Rect2D &geometry) override
		{
			if (_elements.empty())
			{
				return;
			}
			const uint32_t extent = _primary(geometry.size);
			const uint32_t padding = _primary(_elementPadding);
			const double paddingTotal = static_cast<double>(_elements.size() - 1) * padding;
			const uint32_t budget = extent - static_cast<uint32_t>(std::min<double>(extent, paddingTotal));
			const std::vector<float> sizes = _resolveAxis(_primaryHints(), static_cast<float>(budget), Horizontal);
			double resolvedEdge = 0.0;
			uint32_t allocated = 0;
			uint32_t offset = 0;
			const int32_t origin = Horizontal ? geometry.x : geometry.y;
			for (std::size_t i = 0; i < _elements.size(); ++i)
			{
				// Round shared boundaries so fractional pixels are carried into
				// the next cell instead of being allocated more than once.
				resolvedEdge += std::max(0.0, static_cast<double>(sizes[i]));
				const uint32_t edge = static_cast<uint32_t>(std::round(std::min<double>(budget, resolvedEdge)));
				const uint32_t primarySize = edge - allocated;
				const int32_t cursor = static_cast<int32_t>(static_cast<int64_t>(origin) + offset);
				_elements[i]->setGeometry(_cell(geometry, cursor, primarySize));
				allocated = edge;
				offset += primarySize;
				if (i + 1 < _elements.size())
				{
					offset += std::min(padding, extent - offset);
				}
			}
		}

	public:
		LinearLayout()
		{
			_updateSizeHint();
		}

		Element *addWidget(Widget *widget, SizeSettings sizeSettings = {})
		{
			auto element = _createElement(widget, sizeSettings);
			Element *result = element.get();
			_elements.push_back(std::move(element));
			updateSizeHint();
			return result;
		}

		Element *addLayout(Layout *layout, SizeSettings sizeSettings = {})
		{
			auto element = _createElement(layout, sizeSettings);
			Element *result = element.get();
			_elements.push_back(std::move(element));
			updateSizeHint();
			return result;
		}

		void removeElement(Element *element)
		{
			_eraseElement(element);
		}

		void removeWidget(Widget *widget)
		{
			const auto it = std::find_if(_elements.begin(), _elements.end(), [widget](const auto &element) {
				return element->widget() == widget;
			});
			if (it != _elements.end())
			{
				_eraseElement(it->get());
			}
		}

		void removeLayout(Layout *layout)
		{
			const auto it = std::find_if(_elements.begin(), _elements.end(), [layout](const auto &element) {
				return element->layout() == layout;
			});
			if (it != _elements.end())
			{
				_eraseElement(it->get());
			}
		}
	};

	using HorizontalLayout = LinearLayout<Orientation::Horizontal>;
	using VerticalLayout = LinearLayout<Orientation::Vertical>;
}
