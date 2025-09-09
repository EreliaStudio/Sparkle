#pragma once

#include "structure/spk_safe_pointer.hpp"

#include "structure/graphics/spk_geometry_2d.hpp"
#include "structure/math/spk_vector2.hpp"

#include "widget/spk_widget.hpp"

namespace spk
{
	class Layout
	{
	public:
		enum class SizePolicy
		{
			Fixed,
			Minimum,
			Maximum,
			Extend,
			HorizontalExtend,
			VerticalExtend
		};

		class Element
		{
			friend class Layout;

		private:
			spk::SafePointer<spk::Widget> _widget;
			spk::SafePointer<spk::Layout> _layout;
			SizePolicy _sizePolicy;
			spk::Vector2UInt _size;

		public:
			Element();
			Element(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy, const spk::Vector2UInt &p_size);
			Element(spk::SafePointer<spk::Layout> p_layout, const SizePolicy &p_sizePolicy, const spk::Vector2UInt &p_size);

			const spk::SafePointer<spk::Widget> &widget() const;
			const spk::SafePointer<spk::Layout> &layout() const;

			bool isWidget() const;
			bool isLayout() const;

			spk::Vector2UInt minimalSize() const;
			spk::Vector2UInt maximalSize() const;
			void setGeometry(const spk::Geometry2D &p_geometry);

			void setSizePolicy(const SizePolicy &p_sizePolicy);
			const SizePolicy &sizePolicy() const;

			void setSize(const spk::Vector2UInt &p_size);
			const spk::Vector2UInt &size() const;
		};

	protected:
		std::vector<std::unique_ptr<Element>> _elements;

		spk::Vector2UInt _elementPadding{0, 0};

		void resizeElements(const size_t &p_size);

	public:
		virtual void setGeometry(const spk::Geometry2D &p_geometry) = 0;

		virtual spk::Vector2UInt minimalSize() const;
		virtual spk::Vector2UInt maximalSize() const;

		void clear();

		spk::SafePointer<Element> addWidget(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy = SizePolicy::Extend);
		spk::SafePointer<Element> addLayout(spk::SafePointer<spk::Layout> p_layout, const SizePolicy &p_sizePolicy = SizePolicy::Extend);
		void removeWidget(spk::SafePointer<Element> p_element);
		void removeWidget(spk::SafePointer<spk::Widget> p_widget);
		void removeLayout(spk::SafePointer<spk::Layout> p_layout);

		void setElementPadding(const spk::Vector2UInt &p_padding);
		const spk::Vector2UInt &elementPadding() const;

		const std::vector<std::unique_ptr<Element>> &elements() const;
		std::vector<std::unique_ptr<Element>> &elements();
	};
}
