#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "exception.hpp"
#include "graphics/opengl/gpu_resource.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	struct ResourceCounters
	{
		int creations = 0;
		int destructions = 0;
		int synchronizations = 0;
		int bindings = 0;
		bool failCreation = false;
		bool failSynchronization = false;
		bool failBinding = false;
		std::function<void()> onDestruction;
	};

	class FakeGPUResource final : public spk::GPUResource
	{
	private:
		class FakeInstance final : public Instance
		{
		private:
			std::shared_ptr<ResourceCounters> _counters;

		public:
			explicit FakeInstance(std::shared_ptr<ResourceCounters> counters) :
				_counters(std::move(counters))
			{
			}

			~FakeInstance() override
			{
				++_counters->destructions;
				if (_counters->onDestruction)
				{
					_counters->onDestruction();
				}
			}
		};

		std::shared_ptr<ResourceCounters> _counters;

		Kind _kind() const noexcept override
		{
			return Kind::Program;
		}

		std::unique_ptr<Instance> _create(spk::RenderContext &) const override
		{
			++_counters->creations;
			if (_counters->failCreation)
			{
				throw std::runtime_error("creation failure");
			}
			return std::make_unique<FakeInstance>(_counters);
		}

		void _synchronize(Instance &, spk::RenderContext &) const override
		{
			++_counters->synchronizations;
			if (_counters->failSynchronization)
			{
				throw std::runtime_error("synchronization failure");
			}
		}

		void _bind(Instance &, spk::RenderContext &) const override
		{
			++_counters->bindings;
			if (_counters->failBinding)
			{
				throw std::runtime_error("binding failure");
			}
		}

	public:
		explicit FakeGPUResource(std::shared_ptr<ResourceCounters> counters) :
			_counters(std::move(counters))
		{
			// A concrete resource publishes its initial CPU-side state before first use.
			validate();
		}
	};

	static_assert(!std::is_copy_constructible_v<FakeGPUResource>);
	static_assert(!std::is_copy_assignable_v<FakeGPUResource>);
	static_assert(std::is_nothrow_move_constructible_v<FakeGPUResource>);
	static_assert(!std::is_move_assignable_v<FakeGPUResource>);
}

TEST(GPUResourceTest, CreationIsLazyAndActivationReusesTheContextInstance)
{
	auto counters = std::make_shared<ResourceCounters>();
	FakeGPUResource resource(counters);
	auto &openGL = sparkle_test::OpenGLTestContext::instance();

	EXPECT_EQ(counters->creations, 0);
	resource.activate(openGL.renderContext());
	resource.activate(openGL.renderContext());
	EXPECT_EQ(counters->creations, 1);
	EXPECT_EQ(counters->synchronizations, 1);
	EXPECT_EQ(counters->bindings, 2);

	const auto generation = resource.generation();
	resource.validate();
	EXPECT_GT(resource.generation(), generation);
	resource.activate(openGL.renderContext());
	EXPECT_EQ(counters->creations, 1);
	EXPECT_EQ(counters->synchronizations, 2);
	EXPECT_EQ(counters->bindings, 3);
}

TEST(GPUResourceTest, ReleasedInstanceIsDestroyedExactlyOnce)
{
	auto counters = std::make_shared<ResourceCounters>();
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	{
		FakeGPUResource resource(counters);
		resource.activate(openGL.renderContext());
	}

	EXPECT_EQ(counters->destructions, 0);
	openGL.surface()._gpuResources().reclaimReleased();
	EXPECT_EQ(counters->destructions, 1);
	openGL.surface()._gpuResources().reclaimReleased();
	EXPECT_EQ(counters->destructions, 1);
}

TEST(GPUResourceTest, CreationAndSynchronizationFailuresCanBeRetriedCoherently)
{
	auto counters = std::make_shared<ResourceCounters>();
	FakeGPUResource resource(counters);
	auto &context = sparkle_test::OpenGLTestContext::instance().renderContext();

	counters->failCreation = true;
	EXPECT_THROW(resource.activate(context), spk::Exception);
	EXPECT_EQ(counters->creations, 1);
	counters->failCreation = false;
	resource.activate(context);
	EXPECT_EQ(counters->creations, 2);
	EXPECT_EQ(counters->synchronizations, 1);

	resource.validate();
	counters->failSynchronization = true;
	EXPECT_THROW(resource.activate(context), spk::Exception);
	EXPECT_EQ(counters->synchronizations, 2);
	counters->failSynchronization = false;
	resource.activate(context);
	EXPECT_EQ(counters->synchronizations, 3);
}

TEST(GPUResourceTest, BindingFailureDoesNotRecreateOrResynchronizeAValidInstance)
{
	auto counters = std::make_shared<ResourceCounters>();
	FakeGPUResource resource(counters);
	auto &context = sparkle_test::OpenGLTestContext::instance().renderContext();

	counters->failBinding = true;
	EXPECT_THROW(resource.activate(context), spk::Exception);
	EXPECT_EQ(counters->creations, 1);
	EXPECT_EQ(counters->synchronizations, 1);
	counters->failBinding = false;
	resource.activate(context);
	EXPECT_EQ(counters->creations, 1);
	EXPECT_EQ(counters->synchronizations, 1);
	EXPECT_EQ(counters->bindings, 2);
}

TEST(GPUResourceTest, MoveTransfersIdentityAndMakesSourceInert)
{
	auto counters = std::make_shared<ResourceCounters>();
	FakeGPUResource source(counters);
	const auto identifier = source.identifier();
	const auto sourceReplacementIdentifier = [&]() {
		FakeGPUResource destination(std::move(source));
		EXPECT_EQ(destination.identifier(), identifier);
		EXPECT_NE(source.identifier(), identifier);
		EXPECT_THROW(source.activate(sparkle_test::OpenGLTestContext::instance().renderContext()), std::logic_error);
		destination.activate(sparkle_test::OpenGLTestContext::instance().renderContext());
		return source.identifier();
	}();

	EXPECT_NE(sourceReplacementIdentifier, identifier);
	sparkle_test::OpenGLTestContext::instance().surface()._gpuResources().reclaimReleased();
	EXPECT_EQ(counters->destructions, 1);
}

TEST(GPUResourceCollectionTest, IndependentContextsReuseThenClearAndRecreateInstances)
{
	auto counters = std::make_shared<ResourceCounters>();
	FakeGPUResource resource(counters);
	spk::Window::Surface first("FakeFirst"), second("FakeSecond");
	spk::RenderContext a{.targetSurface = &first}, b{.targetSurface = &second};
	resource.activate(a);
	resource.activate(a);
	resource.activate(b);
	resource.activate(b);
	EXPECT_EQ(counters->creations, 2);
	EXPECT_EQ(counters->bindings, 4);
	EXPECT_EQ(counters->synchronizations, 2);
	first._gpuResources().clear();
	EXPECT_EQ(counters->destructions, 1);
	resource.activate(a);
	EXPECT_EQ(counters->creations, 3);
	EXPECT_EQ(counters->synchronizations, 3);
	resource.activate(b);
	EXPECT_EQ(counters->creations, 3);
	first._gpuResources().clear();
	second._gpuResources().clear();
	EXPECT_EQ(counters->destructions, 3);
	first._gpuResources().clear();
	second._gpuResources().reclaimReleased();
	EXPECT_EQ(counters->destructions, 3);
}

TEST(GPUResourceCollectionTest, ResourceAndCollectionCanBeDestroyedInEitherOrder)
{
	for (bool resourceFirst : {false, true})
	{
		auto counters = std::make_shared<ResourceCounters>();
		auto resource = std::make_unique<FakeGPUResource>(counters);
		auto surface = std::make_unique<spk::Window::Surface>("FakeLifetime");
		spk::RenderContext context{.targetSurface = surface.get()};
		resource->activate(context);
		if (resourceFirst)
		{
			resource.reset();
			EXPECT_EQ(counters->destructions, 0);
			surface->_gpuResources().reclaimReleased();
			EXPECT_EQ(counters->destructions, 1);
			surface.reset();
		}
		else
		{
			surface.reset();
			EXPECT_EQ(counters->destructions, 1);
			resource.reset();
		}
		EXPECT_EQ(counters->destructions, 1);
	}
}

TEST(GPUResourceCollectionTest, ResourceIdentifiersKeepSameKindInstancesIndependent)
{
	auto first = std::make_shared<ResourceCounters>(), second = std::make_shared<ResourceCounters>();
	FakeGPUResource a(first), b(second);
	EXPECT_NE(a.identifier(), b.identifier());
	spk::Window::Surface surface("FakeKeys");
	spk::RenderContext context{.targetSurface = &surface};
	a.activate(context);
	b.activate(context);
	a.validate();
	a.activate(context);
	b.activate(context);
	EXPECT_EQ(first->creations, 1);
	EXPECT_EQ(second->creations, 1);
	EXPECT_EQ(first->synchronizations, 2);
	EXPECT_EQ(second->synchronizations, 1);
	spk::RenderContext missing{.targetSurface = nullptr};
	EXPECT_THROW(a.activate(missing), std::invalid_argument);
	surface._gpuResources().clear();
	EXPECT_EQ(first->destructions, 1);
	EXPECT_EQ(second->destructions, 1);
}

TEST(GPUResourceCollectionTest, FailureAtEachActivationStageReleasesExactlyOnceAfterRetry)
{
	for (int stage : {0, 1, 2})
	{
		auto counters = std::make_shared<ResourceCounters>();
		spk::Window::Surface surface("FakeFailure");
		spk::RenderContext context{.targetSurface = &surface};
		{
			FakeGPUResource resource(counters);
			counters->failCreation = stage == 0;
			counters->failSynchronization = stage == 1;
			counters->failBinding = stage == 2;
			try
			{
				resource.activate(context);
				FAIL();
			} catch (const spk::Exception &error)
			{
				EXPECT_NE(std::string(error.what()).find(std::to_string(resource.identifier())), std::string::npos);
				EXPECT_NE(error.cause(), nullptr);
			}
			counters->failCreation = counters->failSynchronization = counters->failBinding = false;
			resource.activate(context);
			EXPECT_EQ(counters->creations, stage == 0 ? 2 : 1);
		}
		surface._gpuResources().reclaimReleased();
		surface._gpuResources().clear();
		EXPECT_EQ(counters->destructions, 1);
	}
}

TEST(GPUResourceCollectionTest, DestructionDuringReclamationQueuesFurtherReleasesForNextTraversal)
{
	spk::Window::Surface surface("Mutation");
	spk::RenderContext context{.targetSurface = &surface};
	auto a = std::make_shared<ResourceCounters>(), b = std::make_shared<ResourceCounters>();
	auto first = std::make_unique<FakeGPUResource>(a);
	auto second = std::make_shared<std::unique_ptr<FakeGPUResource>>(std::make_unique<FakeGPUResource>(b));
	first->activate(context);
	(*second)->activate(context);
	a->onDestruction = [second] {
		second->reset();
	};
	first.reset();
	surface._gpuResources().reclaimReleased();
	EXPECT_EQ(a->destructions, 1);
	EXPECT_EQ(b->destructions, 0);
	EXPECT_EQ(*second, nullptr);
	surface._gpuResources().reclaimReleased();
	EXPECT_EQ(b->destructions, 1);
	surface._gpuResources().clear();
	EXPECT_EQ(a->destructions, 1);
	EXPECT_EQ(b->destructions, 1);
	a->onDestruction = {};
}
