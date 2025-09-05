// spk_form_layout.cpp
#include "widget/spk_form_layout.hpp"
#include <algorithm>
#include <numeric>

namespace spk
{
	namespace
	{
		static inline bool hasRenderable(FormLayout::Element *p_e)
		{
			return (p_e != nullptr) && (p_e->widget() != nullptr || p_e->layout() != nullptr);
		}

		static inline uint32_t heightForPolicy(const FormLayout::Element *p_e)
		{
			// Mirrors original switch mapping
			switch (p_e->sizePolicy())
			{
			case FormLayout::SizePolicy::Fixed:
				return p_e->size().y;
			case FormLayout::SizePolicy::Minimum:
			case FormLayout::SizePolicy::HorizontalExtend:
				return p_e->minimalSize().y;
			case FormLayout::SizePolicy::Maximum:
				return p_e->maximalSize().y;
			case FormLayout::SizePolicy::Extend:
			case FormLayout::SizePolicy::VerticalExtend:
				return p_e->minimalSize().y;
			}
			return 0;
		}

		static inline void accumulateColumn(FormLayout::Element *p_e, uint32_t &p_colW, bool &p_colExpandable)
		{
			if (hasRenderable(p_e) == false)
			{
				return;
			}

			const spk::Vector2UInt requested = p_e->size();

			switch (p_e->sizePolicy())
			{
			case FormLayout::SizePolicy::Fixed:
				p_colW = std::max(p_colW, requested.x);
				break;
			case FormLayout::SizePolicy::Minimum:
			case FormLayout::SizePolicy::VerticalExtend:
				p_colW = std::max(p_colW, p_e->minimalSize().x);
				break;
			case FormLayout::SizePolicy::Maximum:
				// Keep exact behavior from your code: clamp by maximum.
				p_colW = std::min(p_colW, p_e->maximalSize().x);
				break;
			case FormLayout::SizePolicy::Extend:
			case FormLayout::SizePolicy::HorizontalExtend:
				p_colExpandable = true;
				break;
			}
		}

		static inline bool elementVertExpandable(const FormLayout::Element *p_e)
		{
			if (p_e == nullptr)
			{
				return false;
			}
			const auto policy = p_e->sizePolicy();
			return (policy == FormLayout::SizePolicy::Extend || policy == FormLayout::SizePolicy::VerticalExtend);
		}

		// Distribute an integer "extra" fairly across N slots flagged as expandable.
		template <typename AddFn>
		static inline void distribute(uint32_t p_extra, uint32_t p_count, AddFn &&p_addOnce)
		{
			if (p_count == 0U || p_extra == 0U)
			{
				return;
			}
			const uint32_t share = p_extra / p_count;
			uint32_t rem = p_extra % p_count;
			for (uint32_t i = 0; i < p_count; ++i)
			{
				uint32_t add = share + ((rem-- > 0U) ? 1U : 0U);
				if (add > 0U)
				{
					p_addOnce(add);
				}
			}
		}

		struct ScanResult
		{
			uint32_t labelColWidth = 0;
			uint32_t fieldColWidth = 0;
			bool labelColExpandable = false;
			bool fieldColExpandable = false;
			std::vector<int> rowHeights;	 // per-row target height
			std::vector<bool> rowExpandable; // per-row expand flag
		};

		static inline ScanResult scanRowsAndColumns(spk::FormLayout &p_self, std::size_t p_rowCount)
		{
			ScanResult r;
			r.rowHeights.resize(p_rowCount, 0);
			r.rowExpandable.resize(p_rowCount, false);

			for (std::size_t row = 0; row < p_rowCount; ++row)
			{
				auto *label = p_self.elements()[2 * row].get();
				auto *field = p_self.elements()[2 * row + 1].get();

				accumulateColumn(label, r.labelColWidth, r.labelColExpandable);
				accumulateColumn(field, r.fieldColWidth, r.fieldColExpandable);

				const uint32_t hLabel = hasRenderable(label) ? heightForPolicy(label) : 0U;
				const uint32_t hField = hasRenderable(field) ? heightForPolicy(field) : 0U;
				r.rowHeights[row] = static_cast<int>(std::max(hLabel, hField));

				r.rowExpandable[row] = (elementVertExpandable(label) || elementVertExpandable(field));
			}

			return r;
		}

		static inline void ensureSomethingExpands(bool &p_labelExp, bool &p_fieldExp)
		{
			if (!p_labelExp && !p_fieldExp)
			{
				p_labelExp = true;
				p_fieldExp = true;
			}
		}

		static inline void ensureSomeRowsExpand(std::vector<bool> &p_rowExp)
		{
			const bool any = std::any_of(p_rowExp.begin(), p_rowExp.end(), [](bool p_v) { return p_v; });
			if (!any)
			{
				std::fill(p_rowExp.begin(), p_rowExp.end(), true);
			}
		}

		static inline void distributeExtraWidth(uint32_t p_extraWidth, bool p_labelExp, bool p_fieldExp, uint32_t &p_labelW, uint32_t &p_fieldW)
		{
			const uint32_t expandables = static_cast<uint32_t>(p_labelExp) + static_cast<uint32_t>(p_fieldExp);
			if (expandables == 0U || p_extraWidth == 0U)
			{
				return;
			}

			uint32_t share = p_extraWidth / expandables;
			uint32_t rem = p_extraWidth % expandables;

			if (p_labelExp)
			{
				p_labelW += share + ((rem-- > 0U) ? 1U : 0U);
			}
			if (p_fieldExp)
			{
				p_fieldW += share + ((rem-- > 0U) ? 1U : 0U);
			}
		}

		static inline void distributeExtraHeight(uint32_t p_extraHeight, const std::vector<bool> &p_rowExp, std::vector<int> &p_rowHeights)
		{
			if (p_extraHeight == 0U)
			{
				return;
			}

			const uint32_t cnt = static_cast<uint32_t>(std::count(p_rowExp.begin(), p_rowExp.end(), true));
			if (cnt == 0U)
			{
				return;
			}

			uint32_t share = p_extraHeight / cnt;
			uint32_t rem = p_extraHeight % cnt;

			for (std::size_t i = 0; i < p_rowHeights.size(); ++i)
			{
				if (p_rowExp[i])
				{
					p_rowHeights[i] += static_cast<int>(share + ((rem-- > 0U) ? 1U : 0U));
				}
			}
		}

		static inline void placeAllCells(
			spk::FormLayout &p_self, const spk::Geometry2D &p_g, uint32_t p_labelW, uint32_t p_fieldW, const std::vector<int> &p_rowHeights)
		{
			uint32_t y = p_g.anchor.y;

			for (std::size_t row = 0; row < p_rowHeights.size(); ++row)
			{
				const uint32_t h = static_cast<uint32_t>(p_rowHeights[row]);

				spk::Geometry2D labelCell({p_g.anchor.x, y}, {p_labelW, h});
				spk::Geometry2D fieldCell({p_g.anchor.x + p_labelW + p_self.elementPadding().x, y}, {p_fieldW, h});

				auto *label = p_self.elements()[2 * row].get();
				if (hasRenderable(label))
				{
					label->setGeometry(labelCell);
				}

				auto *field = p_self.elements()[2 * row + 1].get();
				if (hasRenderable(field))
				{
					field->setGeometry(fieldCell);
				}

				y += h + p_self.elementPadding().y;
			}
		}
	} // unnamed namespace

	FormLayout::FormElement FormLayout::addRow(
		spk::SafePointer<spk::Widget> p_labelWidget,
		spk::SafePointer<spk::Widget> p_fieldWidget,
		const SizePolicy &p_labelPolicy,
		const SizePolicy &p_fieldPolicy)
	{
		return FormElement{addWidget(p_labelWidget, p_labelPolicy), addWidget(p_fieldWidget, p_fieldPolicy)};
	}

	void FormLayout::removeRow(const FormElement &p_row)
	{
		removeWidget(p_row.labelElement);
		removeWidget(p_row.fieldElement);
	}

	size_t FormLayout::nbRow() const
	{
		return _elements.size() / 2;
	}

	void FormLayout::setGeometry(const spk::Geometry2D &p_geometry)
	{
		const std::size_t rowCount = nbRow();
		if (rowCount == 0U)
		{
			return;
		}

		ScanResult scan = scanRowsAndColumns(*this, rowCount);

		uint32_t labelW = scan.labelColWidth;
		uint32_t fieldW = scan.fieldColWidth;

		const uint32_t minTotalWidth = labelW + fieldW + _elementPadding.x;
		const uint32_t rowsSum = static_cast<uint32_t>(std::accumulate(scan.rowHeights.begin(), scan.rowHeights.end(), 0));
		const uint32_t gaps = static_cast<uint32_t>((rowCount > 0U ? rowCount - 1U : 0U) * _elementPadding.y);
		const uint32_t minTotalHeight = rowsSum + gaps;

		const uint32_t extraW = (p_geometry.size.x > minTotalWidth) ? (p_geometry.size.x - minTotalWidth) : 0U;
		const uint32_t extraH = (p_geometry.size.y > minTotalHeight) ? (p_geometry.size.y - minTotalHeight) : 0U;

		ensureSomethingExpands(scan.labelColExpandable, scan.fieldColExpandable);
		distributeExtraWidth(extraW, scan.labelColExpandable, scan.fieldColExpandable, labelW, fieldW);

		ensureSomeRowsExpand(scan.rowExpandable);
		distributeExtraHeight(extraH, scan.rowExpandable, scan.rowHeights);

		placeAllCells(*this, p_geometry, labelW, fieldW, scan.rowHeights);
	}

	spk::Vector2UInt FormLayout::minimalSize() const
	{
		const std::size_t rowCountValue = nbRow();
		if (rowCountValue == 0U)
		{
			return {0U, 0U};
		}

		uint32_t labelColumnWidth = 0U;
		uint32_t fieldColumnWidth = 0U;
		std::vector<uint32_t> rowHeights(rowCountValue, 0U);

		const auto elementMinW = [](Element *p_e) -> uint32_t { return (hasRenderable(p_e) ? p_e->minimalSize().x : 0U); };
		const auto elementMinH = [](Element *p_e) -> uint32_t { return (hasRenderable(p_e) ? p_e->minimalSize().y : 0U); };

		for (std::size_t row = 0; row < rowCountValue; ++row)
		{
			Element *labelElement = _elements[2 * row].get();
			Element *fieldElement = _elements[2 * row + 1].get();

			labelColumnWidth = std::max(labelColumnWidth, elementMinW(labelElement));
			fieldColumnWidth = std::max(fieldColumnWidth, elementMinW(fieldElement));
			rowHeights[row] = std::max(elementMinH(labelElement), elementMinH(fieldElement));
		}

		const uint32_t minimumTotalWidth = labelColumnWidth + fieldColumnWidth + _elementPadding.x;
		const uint32_t minimumTotalHeight = static_cast<uint32_t>(std::accumulate(rowHeights.begin(), rowHeights.end(), 0U)) +
											static_cast<uint32_t>((rowCountValue > 0U ? rowCountValue - 1U : 0U) * _elementPadding.y);

		return {minimumTotalWidth, minimumTotalHeight};
	}
}
