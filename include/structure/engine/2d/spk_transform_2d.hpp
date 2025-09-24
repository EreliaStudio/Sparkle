#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_matrix.hpp"     // Matrix4x4
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	class Entity2D;
	struct UpdateEvent;

	class Transform2D : public Component
	{
		friend class Entity2D;

	private:
		spk::Matrix4x4 _model;
		spk::Matrix4x4 _inverseModel;

		spk::Vector2   _localPosition{0.0f, 0.0f};
		spk::Vector2   _position{0.0f, 0.0f};
		float          _rotationDeg = 0.0f;         // degrees
		spk::Vector2   _scale{1.0f, 1.0f};
		float          _layer = 0.0f;               // Z translation for render order

		spk::Vector2   _velocity{0.0f, 0.0f};       // unit / millisecond

		spk::Vector2   _forward{0.0f, 1.0f};        // local +Y
		spk::Vector2   _right{1.0f, 0.0f};          // local +X

		ContractProvider _onEditContractProvider;

	public:
		Transform2D();

		ContractProvider::Contract addOnEditionCallback(const std::function<void()>& p_callback);

		const spk::Matrix4x4& model() const;
		const spk::Matrix4x4& inverseModel() const;

		const spk::Vector2& position() const;
		const spk::Vector2& localPosition() const;
		float rotation() const;
		const spk::Vector2& scale() const;
		float layer() const;

		const spk::Vector2& forward() const;
		const spk::Vector2& right() const;

		void lookAt(const spk::Vector2& p_target);
		void lookAtLocal(const spk::Vector2& p_target);

		void setVelocity(const spk::Vector2& p_velocity);
		void addVelocity(const spk::Vector2& p_delta);

		void move(const spk::Vector2& p_delta);
		void place(const spk::Vector2& p_newPosition);

		void rotate(float p_deltaDegrees);
		void setRotation(float p_degrees);

		void setScale(const spk::Vector2& p_scale);
		void setLayer(float p_layer);

		void onUpdateEvent(spk::UpdateEvent& p_event) override;

	private:
		void _updateModel();
	};
}
