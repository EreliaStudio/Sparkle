#pragma once

#include "engine/reference_frame.hpp"
#include "engine/registry.hpp"
#include "engine/system_participant3d.hpp"
#include "container/cached_data.hpp"
#include "design_pattern/contract_provider.hpp"
#include "math/matrix.hpp"
#include "math/quaternion.hpp"
#include "math/vector3.hpp"

#include <string>

namespace spk
{
	class Entity;
	class Entity3D;

	class Transform3D : public System::Participant3D,
						public Registry<Engine *, Transform3D>::Object
	{
		friend class Entity3D;

	public:
		using OnEditionContractProvider = ContractProvider<const Transform3D &>;
		using OnEditionCallback = OnEditionContractProvider::callback_type;
		using OnEditionContract = OnEditionContractProvider::Contract;

	private:
		Vector3 _position = {0.0f, 0.0f, 0.0f};
		Vector3 _scale = {1.0f, 1.0f, 1.0f};
		Quaternion _rotation = Quaternion::identity();

		OnEditionContractProvider _onEditionContractProvider;

		[[nodiscard]] const Transform3D *_parentTransform() const;

		CachedData<Vector3> _worldPosition;
		CachedData<Vector3> _worldScale;
		CachedData<Quaternion> _worldRotation;
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
		explicit Transform3D(const std::string &name);

		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback);

		[[nodiscard]] const Vector3 &position(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] const Vector3 &scale(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] const Quaternion &rotation(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;
		[[nodiscard]] Vector3 eulerRotation(ReferenceFrame referenceFrame = ReferenceFrame::Local) const;

		void place(const Vector3 &position);
		void move(const Vector3 &delta);
		void rescale(const Vector3 &scale);
		void setRotation(const Quaternion &rotation);
		void setEulerRotation(const Vector3 &rotation);
		void rotate(const Quaternion &rotation);
		void rotate(const Vector3 &rotation);
		void rotate(const Vector3 &axis, float angle);

		[[nodiscard]] const Matrix4x4 &localModelMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseLocalModelMatrix() const;
		[[nodiscard]] const Matrix4x4 &modelMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseModelMatrix() const;
	};
}
