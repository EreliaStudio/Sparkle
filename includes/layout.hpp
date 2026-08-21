#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "horizontal_alignment.hpp"
#include "rect2d.hpp"
#include "resizeable_trait.hpp"
#include "vertical_alignment.hpp"

namespace spk
{
	class Widget;

	class Layout : public ResizeableTrait
	{
	public:
		enum class SizePolicy
		{
			Fixed,
			Extend,
			HorizontalExtend,
			VerticalExtend
		};

		class Element
		{
			friend class Layout;

		private:
			Layout *_owner = nullptr;
			Widget *_widget = nullptr;
			Layout *_layout = nullptr;
			ResizeableTrait *_resizeable = nullptr;
			SizePolicy _sizePolicy = SizePolicy::Extend;
			HorizontalAlignment _horizontalAlignment = HorizontalAlignment::Left;
			VerticalAlignment _verticalAlignment = VerticalAlignment::Top;
			ResizeableTrait::Contract _sizeHintEditionContract;

			Element(Layout &owner, Widget *widget, SizePolicy sizePolicy);
			Element(Layout &owner, Layout *layout, SizePolicy sizePolicy);

		public:
			Element(const Element &) = delete;
			Element &operator=(const Element &) = delete;
			Element(Element &&) = delete;
			Element &operator=(Element &&) = delete;
			~Element() = default;

			[[nodiscard]] Widget *widget() const noexcept;
			[[nodiscard]] Layout *layout() const noexcept;
			[[nodiscard]] bool isWidget() const noexcept;
			[[nodiscard]] bool isLayout() const noexcept;

			[[nodiscard]] SizeHint sizeHint() const;
			[[nodiscard]] Vector2 minimalSize() const;
			[[nodiscard]] Vector2 maximalSize() const;
			[[nodiscard]] Vector2 preferredSize() const;

			void setSizePolicy(SizePolicy sizePolicy);
			[[nodiscard]] SizePolicy sizePolicy() const noexcept;

			void setHorizontalAlignment(HorizontalAlignment alignment);
			[[nodiscard]] HorizontalAlignment horizontalAlignment() const noexcept;
			void setVerticalAlignment(VerticalAlignment alignment);
			[[nodiscard]] VerticalAlignment verticalAlignment() const noexcept;

			void setGeometry(const Rect2D &cell) const;
		};

	protected:
		struct AxisSizeHint
		{
			float minimal = 0.0f;
			float preferred = 0.0f;
			float maximal = 0.0f;
		};

		using ResizeableTrait::setMaximalSize;
		using ResizeableTrait::setMinimalSize;
		using ResizeableTrait::setPreferredSize;
		using ResizeableTrait::setSizeHint;

		std::vector<std::unique_ptr<Element>> _elements;
		Vector2UInt _elementPadding{0, 0};

		[[nodiscard]] std::unique_ptr<Element> _createElement(Widget *widget, SizePolicy sizePolicy);
		[[nodiscard]] std::unique_ptr<Element> _createElement(Layout *layout, SizePolicy sizePolicy);
		void _eraseElement(Element *element);

		[[nodiscard]] static SizeHint _normalizeSizeHint(const SizeHint &hint);
		[[nodiscard]] static AxisSizeHint _horizontalHint(const SizeHint &hint) noexcept;
		[[nodiscard]] static AxisSizeHint _verticalHint(const SizeHint &hint) noexcept;
		[[nodiscard]] static std::vector<float> _resolveAxis(const std::vector<AxisSizeHint> &hints, float availableSize);
		[[nodiscard]] static uint32_t _dimension(float value) noexcept;
		[[nodiscard]] static Rect2D _rect(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept;

		void _setComputedSizeHint(const SizeHint &hint);
		virtual void _updateSizeHint() = 0;
		virtual void _applyGeometry(const Rect2D &geometry) = 0;

	public:
		virtual ~Layout() = default;

		void updateSizeHint();
		void setGeometry(const Rect2D &geometry);

		virtual void clear();
		void setElementPadding(const Vector2UInt &padding);
		[[nodiscard]] const Vector2UInt &elementPadding() const noexcept;
		[[nodiscard]] const std::vector<std::unique_ptr<Element>> &elements() const noexcept;
	};
}
