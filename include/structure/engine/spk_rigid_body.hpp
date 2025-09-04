#pragma once

#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <vector>

namespace spk
{
	class RigidBody : public spk::Component
	{
	private:
		ContractProvider::Contract _onOwnerOnTransformEditionContract;
		spk::SafePointer<const spk::CollisionMesh> _collisionMesh;
		static inline std::vector<spk::SafePointer<RigidBody>> _rigidBodies;
		static inline std::mutex _rigidBodiesMutex;

		std::vector<spk::SafePointer<const RigidBody>> _executeCollisionTest();

	public:
		RigidBody(const std::wstring &p_name);
		~RigidBody() override;

		void start() override;
		void awake() override;
		void sleep() override;

		void setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh> &p_collisionMesh);
		const spk::SafePointer<const spk::CollisionMesh> &collisionMesh() const;

		static const std::vector<spk::SafePointer<RigidBody>>& getRigidBodies();

		bool intersect(const spk::SafePointer<RigidBody> p_other) const;
	};
}
