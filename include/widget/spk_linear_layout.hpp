#pragma once

#include "widget/spk_layout.hpp"

#include "spk_enums.hpp"

namespace spk
{
	template <spk::Orientation Orient>
	class LinearLayout : public Layout
	{
	public:
		LinearLayout() = default;

		void setGeometry(const spk::Geometry2D &p_geometry) override
		{
			if (_elements.empty())
				return;

			const bool isHorizontal = (Orient == spk::Orientation::Horizontal);
			const size_t count = _elements.size();

			std::vector<int> primarySize(count, 0);
			std::vector<bool> isExt(count, false);

			for (size_t i = 0; i < count; ++i)
			{
				Element *elt = _elements[i].get();
				if (elt == nullptr || elt->widget() == nullptr)
					continue;

				int requested = (isHorizontal ? elt->size().x : elt->size().y);

				spk::Vector2Int minSize = elt->widget()->minimalSize();
				spk::Vector2Int maxSize = elt->widget()->maximalSize();

				switch (elt->sizePolicy())
				{
					case SizePolicy::Fixed:
					{
						primarySize[i] = requested;
						break;
					}
					case SizePolicy::Minimum:
					{
						primarySize[i] = (isHorizontal ? minSize.x : minSize.y);
						break;
					}
					case SizePolicy::Maximum:
					{
						primarySize[i] = (isHorizontal ? maxSize.x : maxSize.y);
						break;
					}
					case SizePolicy::Extend:
					{
						isExt[i] = true;
						primarySize[i] = requested;
						break;
					}
					case SizePolicy::HorizontalExtend:
					{
						if (isHorizontal == true)
						{
							isExt[i]       = true;
							primarySize[i] = requested;
						}
						else
						{
							primarySize[i] = minSize.y;
						}
						break;
					}
					case SizePolicy::VerticalExtend:
					{
						if (isHorizontal == false)
						{
							isExt[i]       = true;
							primarySize[i] = requested;
						}
						else
						{
							primarySize[i] = minSize.x;
						}
						break;
					}
				}
			}

			int minTotal = std::accumulate(primarySize.begin(), primarySize.end(), 0);
			int paddingTotal = static_cast<int>((count > 0 ? count - 1 : 0) * (isHorizontal == true ? _elementPadding.x : _elementPadding.y));
			int avail = (isHorizontal ? p_geometry.size.x : p_geometry.size.y);
			int extra = std::max(0, avail - (minTotal + paddingTotal));

			size_t nbExt = static_cast<size_t>(std::count(isExt.begin(), isExt.end(), true));
			if (nbExt == 0)
			{
				std::fill(isExt.begin(), isExt.end(), true);
				nbExt = count;
			}

			int share = extra / static_cast<int>(nbExt);
			int remain = extra % static_cast<int>(nbExt);

			if (share > 0)
			{
				for (size_t i = 0; i < count; ++i)
				{
					if (isExt[i])
					{
						primarySize[i] += share + (remain-- > 0 ? 1 : 0);
					}
				}
			}

			const int pad = (isHorizontal ? _elementPadding.x : _elementPadding.y);
			int cursor = (isHorizontal ? p_geometry.anchor.x : p_geometry.anchor.y);
			for (size_t i = 0; i < count; ++i)
			{
				Element *elt = _elements[i].get();

				if (elt != nullptr && elt->widget() != nullptr)
				{
					spk::Vector2Int pos = p_geometry.anchor;
					spk::Vector2Int size = p_geometry.size;

					if (isHorizontal)
					{
						pos.x = cursor;
						size.x = primarySize[i];
					}
					else
					{
						pos.y = cursor;
						size.y = primarySize[i];
					}

					elt->widget()->setGeometry({pos, size});

					cursor += ((isHorizontal == true ? size.x : size.y) > 0 ? pad : 0);
				}
				
				cursor += primarySize[i];
			}
		}
	};

	using HorizontalLayout = LinearLayout<spk::Orientation::Horizontal>;
	using VerticalLayout = LinearLayout<spk::Orientation::Vertical>;
}