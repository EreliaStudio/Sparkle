#include "map_editor_manager.hpp"

MapEditorManager::MapEditorManager(const std::wstring& p_name) :
	spk::Component(p_name)
{
	
}

void MapEditorManager::setCameraManager(spk::SafePointer<CameraManager> p_cameraManager)
{
	_cameraManager = p_cameraManager;
}

void MapEditorManager::setOnClickLambda(const MouseEventLambda& p_lambda)
{
	_onClickLambda = p_lambda;
}

void MapEditorManager::onMouseEvent(spk::MouseEvent& p_event)
{
	static bool leftClicked = false;

	switch (p_event.type)
	{
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				leftClicked = true;

				spk::Vector2Int currentPosition = spk::Vector2::floor(_cameraManager->convertScreenToWorldPosition(p_event.mouse->position).xy());
				_lastMousePosition = currentPosition;
				_onClickLambda(currentPosition);
			}
			break;
		}
		case spk::MouseEvent::Type::Motion:
		{			
			if (leftClicked == false)
				break;

			spk::Vector2Int currentPosition = spk::Vector2::floor(_cameraManager->convertScreenToWorldPosition(p_event.mouse->position).xy());

			if (currentPosition != _lastMousePosition)
			{
				_onClickLambda(currentPosition);
				_lastMousePosition = currentPosition;
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				leftClicked = false;
			}
			break;
		}
	}

}