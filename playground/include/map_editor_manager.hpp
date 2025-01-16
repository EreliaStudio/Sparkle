#pragma once

#include "sparkle.hpp"

#include "world_manager.hpp"
#include "camera_manager.hpp"

class MapEditorManager : public spk::Component
{
public:
	using MouseEventLambda = std::function<void(const spk::Vector2Int&)>;
private:
	MouseEventLambda _onClickLambda;
	spk::Vector2Int _lastMousePosition;
	spk::SafePointer<CameraManager> _cameraManager;

public:
	MapEditorManager(const std::wstring& p_name);

	void setCameraManager(spk::SafePointer<CameraManager> p_cameraManager);
	void setOnClickLambda(const MouseEventLambda& p_lambda);

	void onMouseEvent(spk::MouseEvent& p_event) override;
};