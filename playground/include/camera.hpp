#pragma once

#include <sparkle.hpp>

#include "shader_atlas.hpp"

class Camera
{
private:
	ShaderAtlas::Instanciator _shaderAtlasInstanciator;
	spk::OpenGL::UniformBufferObject &_cameraUBO;

	static inline spk::SafePointer<const Camera> _mainCamera = nullptr;
	spk::Vector2 _position;
	spk::Camera _innerCamera;

	spk::Vector2Int _screenSize;
	spk::Vector2Int _cellSizeInPixels = {32, 32};

	void _updateOrthographicView();
	void _updateView();

public:
	Camera();

	void setAsMainCamera() const;
	static spk::SafePointer<const Camera> mainCamera();

	void initialize(const spk::Vector2Int &p_screenSize, const spk::Vector2Int &p_cellSizeInPixels);
	void setScreenSize(const spk::Vector2Int &p_screenSize);
	void setCellSize(const spk::Vector2Int &p_cellSizeInPixels);

	void place(const spk::Vector2 &p_position);

	const spk::Vector2 &position() const;

	spk::Vector2 screenToWorldPosition(const spk::Vector2Int& p_screenPosition) const;
	spk::Vector2 ndcToWorldPosition(const spk::Vector2& p_ndc) const;
};