#pragma once

#include "structure/spk_safe_pointer.hpp"

#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/event/spk_event.hpp"

#include "widget/spk_widget.hpp"

#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

#include <memory>
#include <stdexcept>

namespace spk
{
    class Frame : public Widget
    {
    private:
        NineSliceRenderer _frameRenderer;
		spk::Vector2Int _cornerSize;

    private:
        virtual void _onGeometryChange() override;

        virtual void _onPaintEvent(PaintEvent& p_event) override;

    public:
        Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

        void setNineSlice(const SafePointer<const spk::SpriteSheet>& p_spriteSheet);

        void setCornerSize(const spk::Vector2Int& p_cornerSize);

        const spk::Vector2Int& cornerSize() const;
    };
}
