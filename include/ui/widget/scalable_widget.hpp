#pragma once

#include <cstdint>
#include <string>

#include "ui/widget.hpp"

namespace spk
{
	class ScalableWidget : public Widget
	{
	public:
		enum Edge : std::uint8_t
		{
			None = 0,
			Left = 1,
			Right = 2,
			Top = 4,
			Bottom = 8
		};

		using Edges = std::uint8_t;

	private:
		static constexpr unsigned int DefaultGrabOffset = 5;

		Edges _activeEdges = None;
		Edges _hoveredEdges = None;
		Rect2D _baseGeometry{};
		Vector2Int _resizeStartPosition{};
		unsigned int _grabOffset = DefaultGrabOffset;
		bool _applyingConstraints = false;
		ResizeableTrait::Contract _sizeHintEditionContract;

		[[nodiscard]] static unsigned int _dimension(float value) noexcept;
		[[nodiscard]] Rect2D _constrainedGeometry(const Rect2D &geometry) const noexcept;
		[[nodiscard]] Rect2D _resizedGeometry(const Vector2Int &position) const noexcept;
		[[nodiscard]] Edges _edgesAt(const Vector2Int &position) const noexcept;
		void _applyGeometryConstraints();
		void _beginResize(EventBase &event, Mouse::Button button, const Vector2Int &position);
		void _endResize(EventBase &event);

	protected:
		void _onGeometryChange() override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;
		void _onMouseLeftEvent(MouseLeftEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;
		void _onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event) override;

	public:
		explicit ScalableWidget(std::string name, Widget *parent = nullptr);

		void setGeometry(const Rect2D &geometry);
		void resize(const Rect2D &geometry);
		void setGrabOffset(unsigned int offset) noexcept;

		[[nodiscard]] bool isResizing() const noexcept;
		[[nodiscard]] Edges activeEdges() const noexcept;
		[[nodiscard]] Edges hoveredEdges() const noexcept;
		[[nodiscard]] unsigned int grabOffset() const noexcept;
		[[nodiscard]] static bool containsEdge(Edges edges, Edge edge) noexcept;
	};
}
