#pragma once

#include <string>

#include "design_pattern/contract_provider.hpp"
#include "type/alignment.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class PopupWidget : public Widget
	{
	public:
		using CloseProvider = ContractProvider<>;
		using CloseCallback = CloseProvider::callback_type;
		using CloseContract = CloseProvider::Contract;

	private:
		Widget *_content = nullptr;
		Widget *_anchorWidget = nullptr;
		Alignment _placement{Alignment::Horizontal::Center, Alignment::Vertical::Bottom};
		Vector2Int _offset{0, 0};
		bool _closeOnOutsidePress = true;
		bool _closeOnEscape = true;
		bool _constrainToRoot = true;
		bool _open = false;
		Widget *_coordinatorRoot = nullptr;
		CloseProvider _closeProvider;
		InherenceTrait<Widget, WidgetChildComparator>::OnParentEditionContract _contentParentContract;

		[[nodiscard]] Rect2D _placedGeometry(const Vector2Int *explicitPosition = nullptr) const;
		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onKeyPressedEvent(KeyPressedEvent &event) override;
		void _onPassiveKeyPressedEvent(KeyPressedEvent &event) override;

	public:
		explicit PopupWidget(std::string name, Widget *parent = nullptr);
		~PopupWidget() override;
		void setContent(Widget *content);
		void setAnchorWidget(Widget *anchor);
		void setPlacement(Alignment placement);
		void setOffset(const Vector2Int &offset);
		void setCloseOnOutsidePress(bool enabled);
		void setCloseOnEscape(bool enabled);
		void setConstrainToRoot(bool enabled);
		void open();
		void openAt(const Vector2Int &rootPosition);
		void close();
		[[nodiscard]] bool isOpen() const noexcept;
		[[nodiscard]] Widget *content() noexcept;
		[[nodiscard]] const Widget *content() const noexcept;
		[[nodiscard]] Widget *anchorWidget() noexcept;
		[[nodiscard]] const Widget *anchorWidget() const noexcept;
		[[nodiscard]] Alignment placement() const noexcept;
		[[nodiscard]] CloseContract subscribeToClose(CloseCallback callback);
	};
}
