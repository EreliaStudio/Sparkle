#include "game_engine/component/spk_tilemap2D.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
    Tilemap2D::Node::Node()
    {
    }

    Tilemap2D::Node::Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, Type p_type) :
        sprite(p_sprite),
        type(p_type),
        flags(p_flags)
    {
    }

    Tilemap2D::Node::Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, Type p_type, const spk::Vector2Int& p_animationFrameOffset, int p_nbFrame, int p_animationDuration) :
        sprite(p_sprite),
        type(p_type),
        animationFrameOffset(p_animationFrameOffset),
        nbFrame(p_nbFrame),
        animationDuration(p_animationDuration),
        flags(p_flags)
    {
    }
}