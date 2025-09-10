#pragma once

#include <memory>
#include <stdexcept>

#include "structure/design_pattern/spk_activable_object.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/graphics/painter/spk_nine_slice_painter.hpp"
#include "structure/graphics/spk_geometry_2d.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/event/spk_event.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class Frame : public Widget
	{
	private:
		NineSlicePainter _frameRenderer;
		spk::Vector2Int _cornerSize;

	private:
		void _onGeometryChange() override;

		void _onPaintEvent(PaintEvent &p_event) override;

	public:
		Frame(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

		void setNineSlice(const SafePointer<const spk::SpriteSheet> &p_spriteSheet);
		SafePointer<const spk::SpriteSheet> nineslice() const;

		void setCornerSize(const spk::Vector2Int &p_cornerSize);

		const spk::Vector2Int &cornerSize() const;
	};
}
