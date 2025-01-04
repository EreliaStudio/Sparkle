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
#include "structure/math/spk_vector2.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

#include <memory>
#include <stdexcept>

namespace spk
{ 
    class PushButton : public Widget
    {
    private:
		ContractProvider _onClickProvider;

        NineSliceRenderer _releasedRenderer;
        NineSliceRenderer _pressedRenderer;

        SafePointer<SpriteSheet> _releasedSpriteSheet;
        SafePointer<SpriteSheet> _pressedSpriteSheet;

        spk::Vector2Int _pressedOffset;

        bool _isPressed;

        spk::Vector2Int _cornerSize;

        void _prepareRenderers();

    private:
        virtual void _onGeometryChange() override;

    	void _onPaintEvent(spk::PaintEvent& p_event);
    	void _onMouseEvent(spk::MouseEvent& p_event);

    public:
        PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		ContractProvider::Contract subscribe(const std::function<void()>& p_callback);

        void setReleasedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet);

        void setPressedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet);

        void setPressedOffset(const spk::Vector2Int& p_offset);

        const spk::Vector2Int& pressedOffset() const;
    };
}
