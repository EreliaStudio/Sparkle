#pragma once

#include "widget/spk_widget.hpp"
#include "widget/spk_text_label.hpp"
#include "widget/spk_frame.hpp"
#include "widget/spk_push_button.hpp"

namespace spk
{
	class InterfaceWindow : public spk::Widget
	{
	public:
		enum Event
		{
			Minimize,
			Maximize,
			Close
		};
	private:
		static spk::SpriteSheet _defaultIconset;

		class MenuBar : public spk::Widget
		{
			friend class InterfaceWindow;

		private:
			spk::TextLabel _titleLabel;
			spk::PushButton _minimizeButton;
			spk::PushButton _maximizeButton;
			spk::PushButton _closeButton;

			void _onGeometryChange();

		public:
			MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);
		};

		spk::Frame _backgroundFrame;
		MenuBar _menuBar;
		spk::Frame _contentFrame;

		float _menuHeight = 20;

		bool _isMoving = false;
		spk::Vector2Int _positionDelta;

		spk::ContractProvider::Contract _minimizeContract;
		spk::ContractProvider::Contract _maximizeContract;
		spk::ContractProvider::Contract _closeContract;

		bool _isMaximized = false;
		spk::Geometry2D _previousGeometry;

		void _onGeometryChange() override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;

	public:
		InterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		void setMenuHeight(const float& p_menuHeight);

		void minimize();
		void maximize();
		void close();

		spk::ContractProvider::Contract subscribeTo(const InterfaceWindow::Event& p_event, const spk::ContractProvider::Job& p_job);
	};
}