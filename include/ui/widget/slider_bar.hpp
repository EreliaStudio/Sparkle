#pragma once

#include <functional>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "type/orientation.hpp"
#include "ui/widget/panel.hpp"

namespace spk
{
	class SliderBar : public Widget
	{
	public:
		struct Range
		{
			float minimum = 0.0f;
			float maximum = 100.0f;

			[[nodiscard]] bool operator==(const Range &other) const = default;
		};

		using EditionProvider = ContractProvider<float>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;

	private:
		Panel _background;
		Panel _body;
		Orientation _orientation = Orientation::Horizontal;
		bool _dragging = false;
		Vector2Int _dragStartPosition{};
		float _dragStartRatio = 0.0f;
		float _scale = 0.1f;
		float _ratio = 0.0f;
		Range _range{};
		EditionProvider _editionProvider;

		[[nodiscard]] unsigned int _primaryLength() const noexcept;
		[[nodiscard]] unsigned int _crossLength() const noexcept;
		[[nodiscard]] unsigned int _bodyLength() const noexcept;
		[[nodiscard]] unsigned int _travelLength() const noexcept;
		[[nodiscard]] int _primaryCoordinate(const Vector2Int &position) const noexcept;
		void _beginDrag(MouseButtonPressedEvent &event);
		void _updateBodyGeometry();
		void _updateSizeHint() override;
		void _onGeometryChange() override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;

	public:
		explicit SliderBar(std::string name, Widget *parent = nullptr);

		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback);

		void setOrientation(Orientation orientation);
		void setScale(float scale);
		void setRange(float minimum, float maximum);
		void setRange(const Range &range);
		void setRatio(float ratio);
		void setValue(float value);

		[[nodiscard]] Orientation orientation() const noexcept;
		[[nodiscard]] float scale() const noexcept;
		[[nodiscard]] float ratio() const noexcept;
		[[nodiscard]] float value() const noexcept;
		[[nodiscard]] const Range &range() const noexcept;
		[[nodiscard]] bool isDragging() const noexcept;

		[[nodiscard]] Panel &background() noexcept;
		[[nodiscard]] const Panel &background() const noexcept;
		[[nodiscard]] Panel &body() noexcept;
		[[nodiscard]] const Panel &body() const noexcept;
	};
}
