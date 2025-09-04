#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_entity.hpp"
#include "structure/math/spk_bounding_box.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <vector>

namespace spk
{
	class Collider : public spk::Component
	{
	public:
		enum class Mode
		{
			Blocking,
			Trigger
		};

	private:
		ContractProvider::Contract _onOwnerOnTransformEditionContract;
		spk::SafePointer<const spk::CollisionMesh> _collisionMesh;
		spk::BoundingBox _boundingBox;
		std::vector<spk::Polygon> _polygons;
		Mode _mode = Mode::Blocking;
		static inline std::vector<spk::SafePointer<Collider>> _colliders;
		static inline std::mutex _collidersMutex;
		spk::TContractProvider<spk::SafePointer<spk::Entity>> _onCollisionEnterProvider;

		std::vector<spk::SafePointer<const Collider>> _executeCollisionTest();
		void _updateCollisionCache();

	public:
		Collider(const std::wstring &p_name);
		~Collider() override;

		void start() override;
		void awake() override;
		void sleep() override;

		void setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh> &p_collisionMesh);
		const spk::SafePointer<const spk::CollisionMesh> &collisionMesh() const;

		static const std::vector<spk::SafePointer<Collider>> &getColliders();

		bool intersect(const spk::SafePointer<Collider> p_other) const;

		void setMode(const Mode &p_mode);
		const Mode &mode() const;

		using CollisionEnterContract = spk::TContractProvider<spk::SafePointer<spk::Entity>>::Contract;
		using CollisionEnterJob = spk::TContractProvider<spk::SafePointer<spk::Entity>>::Job;
		CollisionEnterContract onCollisionEnter(const CollisionEnterJob &p_job);
	};
}
