#include "camera.hpp"

void Camera::_updateOrthographicView()
{
	if (_cellSizeInPixels.x <= 0 || _cellSizeInPixels.y <= 0)
	{
		GENERATE_ERROR("Cell size in pixels can't be defined to 0 or lower");
	}
	if (_screenSize.x <= 0 || _screenSize.y <= 0)
	{
		GENERATE_ERROR("Screen size can't be defined to 0 or lower");
	}

	spk::Vector2 nbCellOnScreen = spk::Vector2(_screenSize) / spk::Vector2(_cellSizeInPixels);

	_innerCamera.setOrthographic(-nbCellOnScreen.x / 2, nbCellOnScreen.x / 2, -nbCellOnScreen.y / 2, nbCellOnScreen.y / 2, 0.1f, 1000.0f);
	_cameraUBO[L"projectionMatrix"] = _innerCamera.projectionMatrix();
	_cameraUBO.validate();
}

void Camera::_updateView()
{
	_cameraUBO[L"viewMatrix"] = spk::Matrix4x4::translation(spk::Vector3(-_position.x, -_position.y, -10.0f));
	_cameraUBO.validate();
}

Camera::Camera() :
	_cameraUBO(_shaderAtlasInstanciator->ubo(L"CameraUBO"))
{
	place({0, 0});
}

void Camera::setAsMainCamera() const
{
	_mainCamera = this;
}

spk::SafePointer<const Camera> Camera::mainCamera()
{
	return (_mainCamera);
}

void Camera::initialize(const spk::Vector2Int &p_screenSize, const spk::Vector2Int &p_cellSizeInPixels)
{
	_screenSize = p_screenSize;
	_cellSizeInPixels = p_cellSizeInPixels;
	_updateOrthographicView();
}

void Camera::setScreenSize(const spk::Vector2Int &p_screenSize)
{
	_screenSize = p_screenSize;
	_updateOrthographicView();
}

void Camera::setCellSize(const spk::Vector2Int &p_cellSizeInPixels)
{
	_cellSizeInPixels = p_cellSizeInPixels;
	_updateOrthographicView();
}

void Camera::place(const spk::Vector2 &p_position)
{
	_position = p_position;
	_updateView();
}

const spk::Vector2 &Camera::position() const
{
	return (_position);
}

spk::Vector2 Camera::screenToWorldPosition(const spk::Vector2Int &p_screenPosition) const
{
	if (_screenSize.x <= 0 || _screenSize.y <= 0)
	{
		GENERATE_ERROR("Screen size must be set before calling screenToWorldPosition");
	}

	const float ndcX = (p_screenPosition.x / static_cast<float>(_screenSize.x)) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (p_screenPosition.y / static_cast<float>(_screenSize.y)) * 2.0f;

	spk::Vector4 ndcPos(ndcX, ndcY, -1.0f, 1.0f);
	spk::Vector4 camPos4 = _innerCamera.inverseProjectionMatrix() * ndcPos;

	return spk::Vector2(camPos4.x + _position.x, camPos4.y + _position.y);
}

spk::Vector2 Camera::ndcToWorldPosition(const spk::Vector2 &p_ndc) const
{
	if (_screenSize.x <= 0 || _screenSize.y <= 0)
	{
		GENERATE_ERROR("Screen size must be set before calling screenToWorldPosition");
	}

	spk::Vector4 camPos4 = _innerCamera.inverseProjectionMatrix() * spk::Vector4(p_ndc, -1.0f, 1.0f);

	return spk::Vector2(camPos4.x + _position.x, camPos4.y + _position.y);
}