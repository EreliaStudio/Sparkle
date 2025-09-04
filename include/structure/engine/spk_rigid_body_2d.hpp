#pragma once

#include "structure/engine/spk_collision_mesh_2d.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_bounding_box_2d.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <vector>

namespace spk
{
	class RigidBody2D : public spk::Component
	{
	public:
	private:
		spk::SafePointer<const spk::CollisionMesh2D> _collider;
		static inline std::vector<spk::SafePointer<const RigidBody2D>> _rigidBodies;
		static inline std::mutex _rigidBodiesMutex;

	public:
		RigidBody2D(const std::wstring &p_name);
		~RigidBody2D() override;

		void awake() override;
		void sleep() override;

		void setCollider(const spk::SafePointer<const spk::CollisionMesh2D> &p_collider);
		const spk::SafePointer<const spk::CollisionMesh2D> &collider() const;

		static std::vector<spk::SafePointer<RigidBody2D>> getRigidBodies();

		bool intersect(const spk::SafePointer<RigidBody2D> p_other) const;
	};
}
