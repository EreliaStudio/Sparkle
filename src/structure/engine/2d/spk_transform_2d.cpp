#include "structure/engine/2d/spk_transform_2d.hpp"

#include "structure/engine/2d/spk_entity_2d.hpp"

#include "structure/engine/3d/spk_transform.hpp"
#include "structure/engine/3d/spk_entity.hpp"

namespace spk
{
	Transform2D::Transform2D() :
		Component(L"Transform2D")
	{
		_updateModel();
	}

	ContractProvider::Contract Transform2D::addOnEditionCallback(const std::function<void()> &p_callback)
	{
		return std::move(_onEditContractProvider.subscribe(p_callback));
	}

	const spk::Matrix4x4 &Transform2D::model() const
	{
		return _model;
	}
	const spk::Matrix4x4 &Transform2D::inverseModel() const
	{
		return _inverseModel;
	}

	const spk::Vector2 &Transform2D::position() const
	{
		return _position;
	}
	const spk::Vector2 &Transform2D::localPosition() const
	{
		return _localPosition;
	}
	float Transform2D::rotation() const
	{
		return _rotationDeg;
	}
	const spk::Vector2 &Transform2D::scale() const
	{
		return _scale;
	}
	float Transform2D::layer() const
	{
		return _layer;
	}

	const spk::Vector2 &Transform2D::forward() const
	{
		return _forward;
	}
	const spk::Vector2 &Transform2D::right() const
	{
		return _right;
	}

	void Transform2D::lookAt(const spk::Vector2 &p_target)
	{
		const spk::Vector2 d = p_target - _position;
		_rotationDeg = std::atan2(d.y, d.x) * 180.0f / 3.14159265358979323846f;
		_updateModel();
	}

	void Transform2D::lookAtLocal(const spk::Vector2 &p_target)
	{
		const spk::Vector2 d = p_target - _localPosition;
		_rotationDeg = std::atan2(d.y, d.x) * 180.0f / 3.14159265358979323846f;
		_updateModel();
	}

	void Transform2D::setVelocity(const spk::Vector2 &p_velocity)
	{
		_velocity = p_velocity / 1000.0f;
	}
	void Transform2D::addVelocity(const spk::Vector2 &p_delta)
	{
		_velocity += p_delta / 1000.0f;
	}

	void Transform2D::move(const spk::Vector2 &p_delta)
	{
		_localPosition += p_delta;
		_updateModel();
	}
	void Transform2D::place(const spk::Vector2 &p_newPosition)
	{
		_localPosition = p_newPosition;
		_updateModel();
	}

	void Transform2D::rotate(float p_deltaDegrees)
	{
		_rotationDeg += p_deltaDegrees;
		_updateModel();
	}
	void Transform2D::setRotation(float p_degrees)
	{
		_rotationDeg = p_degrees;
		_updateModel();
	}

	void Transform2D::setScale(const spk::Vector2 &p_scale)
	{
		_scale = p_scale;
		_updateModel();
	}
	void Transform2D::setLayer(float p_layer)
	{
		_layer = p_layer;
		_updateModel();
	}

	void Transform2D::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (_velocity != spk::Vector2())
		{
			move(_velocity * static_cast<float>(p_event.deltaTime.milliseconds));
		}
	}

	void Transform2D::_updateModel()
	{
		const spk::Matrix4x4 S = spk::Matrix4x4::scale(spk::Vector3(_scale.x, _scale.y, 1.0f));
		const spk::Matrix4x4 Rz = spk::Matrix4x4::rotateAroundAxis(spk::Vector3(0.0f, 0.0f, 1.0f), _rotationDeg);
		const spk::Matrix4x4 T = spk::Matrix4x4::translation(spk::Vector3(_localPosition.x, _localPosition.y, _layer));

		spk::Matrix4x4 parentModel = spk::Matrix4x4::identity();
		if (owner() != nullptr && owner()->parent() != nullptr)
		{
			if (owner()->parent().isCastable<spk::Entity2D>())
			{
				spk::SafePointer<Transform2D> transform2D = owner()->parent()->getComponent<Transform2D>();
				parentModel = transform2D->model();
			}
			else if (owner()->parent().isCastable<spk::Entity>())
			{			
				spk::SafePointer<Transform> transform = owner()->parent()->getComponent<Transform>();	
				parentModel = transform->model();
			}
		}

		_model = S * Rz * T * parentModel;
		_inverseModel = _model.inverse();

		const spk::Vector2 right2(_model[0][0], _model[0][1]);
		const spk::Vector2 up2(_model[1][0], _model[1][1]);

		_right = right2.normalize();
		_forward = up2.normalize();

		_position = spk::Vector2(_model[3][0], _model[3][1]);

		_onEditContractProvider.trigger();
	}
}