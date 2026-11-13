#include <gtest/gtest.h>

#include <GL/glew.h>

#include <cmath>
#include <cstdint>

#include "engine/entity3d.hpp"
#include "light/point.hpp"
#include "light/spot.hpp"
#include "lighting/system.hpp"
#include "rendering/engine/ambient_light.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

TEST(ForwardLighting, AmbientDefaultsAndLinearHDRCapture)
{
	spk::AmbientLight ambient;
	EXPECT_EQ(ambient.color(), (spk::RGB{1.0f, 1.0f, 1.0f}));
	EXPECT_FLOAT_EQ(ambient.intensity(), 0.0f);
	EXPECT_EQ(ambient.contribution(), (spk::RGB{0, 0, 0}));
	ambient.setColor({0.5f, 0.25f, 1.0f});
	ambient.setIntensity(8.0f);
	EXPECT_EQ(ambient.contribution(), (spk::RGB{4.0f, 2.0f, 8.0f}));

	const spk::Light::Selection empty;
	const spk::Lighting::System captured(empty, ambient.contribution());
	const auto data = captured.buffer().cast<spk::Lighting::System::Header, spk::Lighting::System::LightData>();
	EXPECT_EQ(data.fixedPart.ambient, (spk::RGB{4, 2, 8}));
	EXPECT_EQ(data.fixedPart.count, 0u);
	EXPECT_EQ(data.nbElement, 0u);

	ambient.setIntensity(0.0f);
	EXPECT_EQ(data.fixedPart.ambient, (spk::RGB{4, 2, 8}));
}

TEST(ForwardLighting, CapturedLocalAndDirectionalGPUDataPreserveOrderAndType)
{
	spk::Light::Selection selection;
	auto &point = selection.local.emplace_back();
	point.type = spk::Light::Selection::Type::Point;
	point.position = {1, 2, 3};
	point.range = 9.0f;
	point.color = {1, 0, 0};
	point.intensity = 2.0f;
	auto &spot = selection.local.emplace_back();
	spot.type = spk::Light::Selection::Type::Spot;
	spot.position = {4, 5, 6};
	spot.direction = {0, 0, -1};
	spot.angle = 0.5f;
	spot.range = 12.0f;
	spot.color = {0, 1, 0};
	spot.intensity = 3.0f;
	auto &sun = selection.directional.emplace_back();
	sun.type = spk::Light::Selection::Type::Directional;
	sun.direction = {0, -1, 0};
	sun.color = {0, 0, 1};
	sun.intensity = 4.0f;

	const spk::Lighting::System captured(selection, {0.125f, 0.25f, 0.5f});
	const auto data = captured.buffer().cast<spk::Lighting::System::Header, spk::Lighting::System::LightData>();
	ASSERT_EQ(data.nbElement, 3u);
	EXPECT_EQ(data.fixedPart.count, 3u);
	EXPECT_EQ(data.fixedPart.localCount, 2u);
	EXPECT_EQ(data.fixedPart.directionalCount, 1u);
	EXPECT_EQ(data.dynamicArray[0].type, 0u);
	EXPECT_EQ(data.dynamicArray[0].position, (spk::Vector3{1, 2, 3}));
	EXPECT_FLOAT_EQ(data.dynamicArray[0].range, 9.0f);
	EXPECT_FLOAT_EQ(data.dynamicArray[0].intensity, 2.0f);
	EXPECT_EQ(data.dynamicArray[1].type, 1u);
	EXPECT_FLOAT_EQ(data.dynamicArray[1].coneCosine, std::cos(0.5f));
	EXPECT_EQ(data.dynamicArray[2].type, 2u);
	EXPECT_EQ(data.dynamicArray[2].direction, (spk::Vector3{0, -1, 0}));
	EXPECT_EQ(data.dynamicArray[2].color, (spk::RGB{0, 0, 1}));

	selection.local[0].intensity = 99.0f;
	selection.directional.clear();
	EXPECT_FLOAT_EQ(data.dynamicArray[0].intensity, 2.0f);
	EXPECT_EQ(data.fixedPart.count, 3u);
}

TEST(ForwardLighting, RadialAndAngularAttenuationMatchSelectedLightContracts)
{
	spk::Light::Point point;
	point.setRange(10.0f);
	EXPECT_FLOAT_EQ(point.attenuation(0), 1.0f);
	EXPECT_FLOAT_EQ(point.attenuation(5), 0.5f);
	EXPECT_FLOAT_EQ(point.attenuation(10), 0.0f);
	EXPECT_FLOAT_EQ(point.attenuation(11), 0.0f);
	point.setRange(0.0f);
	EXPECT_FLOAT_EQ(point.attenuation(0), 0.0f);

	spk::Entity3D source{"source"};
	auto &spot = source.addComponent<spk::Light::Spot>();
	spot.setRange(10.0f);
	EXPECT_NEAR(spot.attenuation(5, {0, 0, -5}), 0.5f, 0.0001f);
	EXPECT_NEAR(spot.angularAttenuation({5, 0, 0}), 0.0f, 0.0001f);
	EXPECT_FLOAT_EQ(spot.angularAttenuation({0, 0, -5}), 1.0f);
	spot.setRange(0.0f);
	EXPECT_FLOAT_EQ(spot.attenuation(0, {0, 0, -1}), 0.0f);
}

TEST(ForwardLighting, DrawBindingCanRecoverAfterAnotherLightingPayloadUsesSameIndex)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset({64, 64});
	spk::Light::Selection selection;
	const spk::Lighting::System first(selection, {0.1f, 0.2f, 0.3f});
	const spk::Lighting::System second(selection, {0.7f, 0.8f, 0.9f});

	first.activate(gl.renderContext());
	GLint firstBuffer = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, spk::Lighting::System::BindingPoint, &firstBuffer);
	ASSERT_GT(firstBuffer, 0);
	second.activate(gl.renderContext());
	GLint secondBuffer = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, spk::Lighting::System::BindingPoint, &secondBuffer);
	ASSERT_GT(secondBuffer, 0);
	ASSERT_NE(secondBuffer, firstBuffer);
	first.activate(gl.renderContext());
	GLint reboundBuffer = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, spk::Lighting::System::BindingPoint, &reboundBuffer);
	EXPECT_EQ(reboundBuffer, firstBuffer);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}
