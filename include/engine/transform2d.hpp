#pragma once

#include "container/cached_data.hpp"
#include "design_pattern/contract_provider.hpp"
#include "engine/reference_frame.hpp"
#include "engine/registry.hpp"
#include "engine/system_participant2d.hpp"
#include "math/matrix.hpp"
#include "math/vector2.hpp"

#include <string>

namespace spk
{
	class Entity;
	class Entity2D;

	class Transform2D : public System::Participant2D,
						public Registry<Engine *, Transform2D>::Object
	{
		friend class Entity2D;

	public:
		using OnEditionContractProvider = ContractProvider<const Transform2D &>;
		using OnEditionCallback = OnEditionContractProvider::callback_type;
		using OnEditionContract = OnEditionContractProvider::Contract;

	private:
		Vector2 _position = {0.0f, 0.0f};
		Vector2 _scale = {1.0f, 1.0f};
		float _rotation = 0.0f;
		float _depth = 0.0f;

		OnEditionContractProvider _onEditionContractProvider;

		[[nodiscard]] const Transform2D *_parentTransform() const;

		CachedData<Vector2> _worldPosition;
		CachedData<Vector2> _worldScale;
		CachedData<float> _worldRotation;
		CachedData<float> _worldDepth;
		CachedData<Matrix4x4> _localModelMatrix;
		CachedData<Matrix4x4> _inverseLocalModelMatrix;
		CachedData<Matrix4x4> _modelMatrix;
		CachedData<Matrix4x4> _inverseModelMatrix;

		void _clearWorldCaches();
		void _clearAllCaches();

		static void _clearWorldCachesRecursively(Entity &entity);
		void _clearDescendantWorldCaches();
		void _notifyEdition();

	public:
		explicit Transform2D(const std::string &name);

		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback);

		[[nodiscard]] const Vector2 &position(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] const Vector2 &scale(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] float rotation(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] float depth(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;

		void place(const Vector2 &position);
		void move(const Vector2 &delta);
		void rescale(const Vector2 &scale);
		void setRotation(float rotation);
		void rotate(float delta);
		void setDepth(float depth);
		void elevate(float delta);

		[[nodiscard]] const Matrix4x4 &localModelMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseLocalModelMatrix() const;
		[[nodiscard]] const Matrix4x4 &modelMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseModelMatrix() const;
	};
}
