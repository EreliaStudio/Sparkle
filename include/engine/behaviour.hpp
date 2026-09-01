#pragma once

#include "core/event/event_dispatcher.hpp"
#include "engine/entity_attachment.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

#include <string>

namespace spk
{
	struct UpdateContext;
	class Entity;

	class Behaviour : public EntityAttachment,
					  public EventDispatcher
	{
	private:
		spk::Rect2D _geometry{};

	protected:
		[[nodiscard]] bool _isAcceptingInteraction() const override;

		virtual void _onGeometryChange(const spk::Rect2D &geometry);
		virtual void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		virtual void _updateState(UpdateContext &context);

	public:
		Behaviour(
			const std::string &name = "Unnamed behaviour",
			Entity *owner = nullptr);
		explicit Behaviour(Entity *owner);

		~Behaviour() override = default;

		void handleGeometryChange(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		void updateState(UpdateContext &context);
	};
}
