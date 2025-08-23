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
	public:
		struct BoundingBox
		{
			spk::Vector3 min;
			spk::Vector3 max;
		};

	private:
		spk::SafePointer<const spk::CollisionMesh> _collider;
		static inline std::vector<spk::SafePointer<RigidBody>> _rigidBodies;
		static inline std::mutex _rigidBodiesMutex;

		BoundingBox _computeLocalBoundingBox() const;

	public:
		RigidBody(const std::wstring &p_name);
		~RigidBody() override;

		void start() override;
		void stop() override;

		void setCollider(const spk::SafePointer<const spk::CollisionMesh> &p_collider);
		const spk::SafePointer<const spk::CollisionMesh> &collider() const;

		BoundingBox boundingBox() const;

		static std::vector<spk::SafePointer<RigidBody>> getRigidBodies();
		static bool intersect(const RigidBody *p_a, const spk::Matrix4x4 &p_transformA, const RigidBody *p_b, const spk::Matrix4x4 &p_transformB);
	};
}
