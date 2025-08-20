#pragma once
#include "widget/spk_widget.hpp"

namespace spk
{
    class ScalableWidget : public spk::Widget
    {
    protected:
        spk::Vector2UInt _minimumSize = {0, 0};
        spk::Vector2UInt _maximumSize = {1000, 1000};

        // Bit flags for active/hovered resize edges
        enum Edge : uint8_t { None = 0, Left = 1, Right = 2, Top = 4, Bottom = 8 };
        uint8_t _activeEdges = Edge::None;

        spk::Geometry2D _baseGeometry;
        spk::Vector2Int _positionDelta;

        spk::Geometry2D _topAnchorArea;
        spk::Geometry2D _leftAnchorArea;
        spk::Geometry2D _rightAnchorArea;
        spk::Geometry2D _downAnchorArea;

        void _updateAnchorAreas();

        // --- helpers to reduce complexity ---
        uint8_t _hoverEdges(const spk::Vector2Int& mousePos) const;
        void _setCursorForEdges(uint8_t edges, spk::SafePointer<Window> p_window);
        void _beginResize(uint8_t edges, const spk::Vector2Int& startPos);
        void _endResize(spk::SafePointer<Window> p_window);
        spk::Geometry2D _computeResizedGeometry(const spk::Vector2Int& currentPos) const;

        virtual void _onMouseEvent(spk::MouseEvent &p_event) override;

    public:
        ScalableWidget(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

        spk::Vector2UInt minimalSize() const override;
        spk::Vector2UInt maximalSize() const override;

        void setMinimumSize(const spk::Vector2UInt &p_minimumSize);
        void setMaximumSize(const spk::Vector2UInt &p_maximumSize);

        void forceGeometryChange(const Geometry2D &p_geometry) override;
        void place(const spk::Vector2Int &p_delta) override;
        void move(const spk::Vector2Int &p_delta) override;
        void setGeometry(const Geometry2D &p_geometry) override;
        void forceGeometryChange(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size) override;
        void setGeometry(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size) override;
    };
}
