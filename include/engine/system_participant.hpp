#pragma once

#include "engine/entity_attachment.hpp"
#include "engine/registry.hpp"
#include "engine/system.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

#include <string>

namespace spk
{
	class Entity;

	class System::Participant : public EntityAttachment,
								public spk::Registry<Engine *, System::Participant>::Object
	{
	private:
		spk::Rect2D _geometry{};

	protected:
		virtual void _onGeometryChange(const spk::Rect2D &geometry);
		virtual void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);

	public:
		Participant(
			const std::string &name = "Unnamed participant",
			Entity *owner = nullptr);
		explicit Participant(Entity *owner);

		~Participant() override = default;

		void handleGeometryChange(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
	};
}
