#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/2d/spk_collision_mesh_2d.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/2d/spk_entity_2d.hpp"
#include "structure/math/spk_bounding_box_2d.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_polygon_2d.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <vector>

namespace spk
{
	class Collider2D : public spk::Component
	{
	public:
		enum class Mode
		{
			Blocking,
			Trigger
		};

	private:
		ContractProvider::Contract _onOwnerOnTransformEditionContract;
		spk::SafePointer<const spk::CollisionMesh2D> _collisionMesh;
		spk::BoundingBox2D _boundingBox;
		std::vector<spk::Polygon2D> _polygons;
		Mode _mode = Mode::Blocking;
		static inline std::vector<spk::SafePointer<Collider2D>> _colliders;
		static inline std::mutex _collidersMutex;
		spk::TContractProvider<spk::SafePointer<spk::GenericEntity>> _onCollisionEnterProvider;

		std::vector<spk::SafePointer<const Collider2D>> _executeCollisionTest();
		void _updateCollisionCache();

	public:
		Collider2D(const std::wstring &p_name);
		~Collider2D() override;

		void start() override;
		void awake() override;
		void sleep() override;

		void setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh2D> &p_collisionMesh);
		const spk::SafePointer<const spk::CollisionMesh2D> &collisionMesh() const;

		static const std::vector<spk::SafePointer<Collider2D>> &getColliders();

		bool intersect(const spk::SafePointer<Collider2D> p_other) const;

		void setMode(const Mode &p_mode);
		const Mode &mode() const;

		using CollisionEnterContract = spk::TContractProvider<spk::SafePointer<spk::GenericEntity>>::Contract;
		using CollisionEnterJob = spk::TContractProvider<spk::SafePointer<spk::GenericEntity>>::Job;
		CollisionEnterContract onCollisionEnter(const CollisionEnterJob &p_job);
	};
}
