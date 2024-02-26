#include "game_engine/spk_tilemap2D.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
    Tilemap2D::Node::Node()
    {
    }

    Tilemap2D::Node::Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, bool p_isAutotiled) :
        sprite(p_sprite),
        isAutotiled(p_isAutotiled),
        flags(p_flags)
    {
    }

    Tilemap2D::Node::Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, bool p_isAutotiled, const spk::Vector2& p_animationFrameOffset, int p_nbFrame, int p_animationDuration) :
        sprite(p_sprite),
        isAutotiled(p_isAutotiled),
        animationFrameOffset(p_animationFrameOffset),
        nbFrame(p_nbFrame),
        animationDuration(p_animationDuration),
        flags(p_flags)
    {
    }
}