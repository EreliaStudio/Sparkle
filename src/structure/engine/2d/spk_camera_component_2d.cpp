#include "structure/engine/2d/spk_camera_component_2d.hpp"
#include <cmath>

namespace spk
{
	CameraComponent2D::CameraComponent2D(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_camera.setOrthographic(-1, 1, -1, 1, _policy.nearPlane, _policy.farPlane);
	}

	void CameraComponent2D::_updateUBO()
	{
		auto &cameraUBO = spk::Lumina::Shader::Constants::ubo(L"CameraUBO");
		const spk::Transform2D &t = owner().upCast<spk::Entity2D>()->transform();

		cameraUBO.layout()[L"viewMatrix"] = t.inverseModel();
		cameraUBO.layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO.validate();
	}

	void CameraComponent2D::_applyProjection()
	{
		if (_geometry.has_value() == false)
		{
			return;
		}

		const auto &g = _geometry.value();
		const float pxW = static_cast<float>(g.size.x);
		const float pxH = static_cast<float>(g.size.y);
		if (pxW <= 0.0f || pxH <= 0.0f)
		{
			return;
		}

		const float ppuX = (_policy.pixelsPerUnit.x != 0.0f) ? _policy.pixelsPerUnit.x : 1.0f;
		const float ppuY = (_policy.pixelsPerUnit.y != 0.0f) ? _policy.pixelsPerUnit.y : 1.0f;
		if (ppuX <= 0.0f || ppuY <= 0.0f)
		{
			GENERATE_ERROR("CameraComponent2D: pixelsPerUnit must be > 0 on both axes");
		}

		const float worldWidth = pxW / ppuX;
		const float worldHeight = pxH / ppuY;
		const float halfW = worldWidth * 0.5f;
		const float halfH = worldHeight * 0.5f;

		_camera.setOrthographic(-halfW, halfW, -halfH, halfH, _policy.nearPlane, _policy.farPlane);
		
		_updateUBO();
	}

	void CameraComponent2D::setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit)
	{
		_policy.pixelsPerUnit = p_pixelsPerUnit;
		_applyProjection();
	}

	void CameraComponent2D::setNearFar(float p_near, float p_far)
	{
		_policy.nearPlane = p_near;
		_policy.farPlane = p_far;
		_applyProjection();
	}

	void CameraComponent2D::onGeometryChange(const spk::Geometry2D &p_geometry)
	{
		_geometry = p_geometry;
		_applyProjection();
	}

	void CameraComponent2D::start()
	{
		_transformContract = owner().upCast<spk::Entity2D>()->transform().addOnEditionCallback([this]() { _updateUBO(); });
		_updateUBO();
	}

	spk::Vector2 CameraComponent2D::screenToWorld(const spk::Vector2Int &p_screenPosition) const
	{
		if (_geometry.has_value() == false)
		{
			GENERATE_ERROR("CameraComponent2D::screenToWorld - geometry not set (onGeometryChange not called yet)");
		}

		const auto &g = _geometry.value();
		const float pxW = static_cast<float>(g.size.x);
		const float pxH = static_cast<float>(g.size.y);
		if (pxW <= 0.0f || pxH <= 0.0f)
		{
			GENERATE_ERROR("CameraComponent2D::screenToWorld - invalid geometry size");
		}

		const float ndcX = (static_cast<float>(p_screenPosition.x) / pxW) * 2.0f - 1.0f;
		const float ndcY = 1.0f - (static_cast<float>(p_screenPosition.y) / pxH) * 2.0f;

		return ndcToWorld({ndcX, ndcY});
	}

	spk::Vector2 CameraComponent2D::ndcToWorld(const spk::Vector2 &p_ndc) const
	{
		return (_camera.convertScreenToCamera(p_ndc).xy());
	}
}
