#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "engine/entity3d.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/render_command.hpp"

namespace sparkle_test::engine_view
{
	class RecordCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::string> *_output;
		std::string _value;

	public:
		RecordCommand(std::vector<std::string> &output, std::string value) :
			_output(&output),
			_value(std::move(value))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_output->push_back(_value);
		}
	};

	class Probe final : public spk::RenderingBehaviour
	{
	public:
		using Callback = std::function<void(spk::RenderSnapshot::Builder &, const spk::EngineView::CaptureContext &)>;
		Callback callback;
		std::size_t contributions = 0;

		void buildViewRenderSnapshot(spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &context) override
		{
			++contributions;
			if (callback)
			{
				callback(builder, context);
			}
		}
	};

	struct Fixture
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"camera"};
		spk::CameraComponent3D &camera;
		std::vector<std::string> log;

		Fixture() :
			camera(cameraEntity.addComponent<spk::CameraComponent3D>())
		{
			engine.addEntity(&cameraEntity);
		}

		~Fixture()
		{
			engine.removeEntity(&cameraEntity);
		}

		spk::EngineView &addView(const std::string &name, std::optional<spk::EngineView::Order> order = std::nullopt)
		{
			return engine.addView(name, &camera, {128, 96}, order);
		}

		Probe &record(const std::string &label, spk::RenderPass::Key key = spk::EngineView::OpaquePassKey)
		{
			auto &probe = engine.root().addBehaviour<Probe>();
			probe.callback = [this, label, key](spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &context) {
				builder.renderPass(key).emplace<RecordCommand>(log, context.name + ":" + label);
			};
			return probe;
		}

		void executeLatest()
		{
			auto snapshot = engine.latestRenderSnapshot();
			if (!snapshot)
			{
				throw std::logic_error("No published snapshot");
			}
			spk::RenderContext context{.targetSurface = nullptr};
			snapshot->execute(context);
		}
	};
}
