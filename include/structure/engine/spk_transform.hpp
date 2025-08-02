#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_quaternion.hpp"
#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	class Entity;

	class Transform : public Component
	{
		friend class Entity;

	private:
		spk::Matrix4x4 _model;
		spk::Matrix4x4 _inverseModel;

		spk::Vector3 _localPosition;
		spk::Vector3 _position;
		spk::Quaternion _rotation;
		spk::Vector3 _scale;

		spk::Vector3 _velocity; // Unit : unit / milliseconds

		spk::Vector3 _forward;
		spk::Vector3 _right;
		spk::Vector3 _up;

		ContractProvider _onEditContractProvider;

	public:
		Transform();

		ContractProvider::Contract addOnEditionCallback(const std::function<void()> &p_callback);

		const spk::Matrix4x4 &model() const;
		const spk::Matrix4x4 &inverseModel() const;

		const spk::Vector3 &position() const;
		const spk::Vector3 &localPosition() const;

		spk::Vector3 rotation() const;
		const spk::Quaternion &rotationQuaternion() const;

		const spk::Vector3 &scale() const;

		const spk::Vector3 &forward() const;
		const spk::Vector3 &right() const;
		const spk::Vector3 &up() const;

		void lookAt(const spk::Vector3 &target);

		void setVelocity(const spk::Vector3 &p_velocity);
		void addVelocity(const spk::Vector3 &p_delta);

		void move(const spk::Vector3 &p_delta);
		void place(const spk::Vector3 &p_newPosition);

		void rotate(const spk::Vector3 &p_deltaEuler);
		void setRotation(const spk::Vector3 &p_euler);
		void rotateAroundPoint(const spk::Vector3 &center, const spk::Vector3 &axis, float angle);

		void setScale(const spk::Vector3 &p_scale);

		void onUpdateEvent(spk::UpdateEvent &p_event) override;

	private:
		void _updateModel();
	};
}
