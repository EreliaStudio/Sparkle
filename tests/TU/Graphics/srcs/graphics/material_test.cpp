#include <gtest/gtest.h>

#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

#include "asset/graphics_factories.hpp"
#include "asset/manager.hpp"
#include "container/json/object.hpp"
#include "exception.hpp"
#include "graphics/image.hpp"
#include "graphics/material.hpp"

namespace
{
	using Value = spk::JSON::Value;

	[[nodiscard]] std::string texturePath()
	{
		return (std::filesystem::path(SPARKLE_TEST_RESOURCES_DIR) / "textures/default_iconset.png").string();
	}

	[[nodiscard]] Value pathSource(bool sheet = false, unsigned int count = 16)
	{
		Value source = Value::object();
		source["path"] = texturePath();
		if (sheet)
		{
			source["size"]["x"] = count;
			source["size"]["y"] = count;
		}
		return source;
	}

	[[nodiscard]] Value assetSource(bool sheet = false, unsigned int count = 16)
	{
		Value source = Value::object();
		source["asset"]["type"] = sheet ? "SpriteSheet" : "Texture";
		source["asset"]["id"] = texturePath();
		if (sheet)
		{
			source["asset"]["config"]["size"]["x"] = count;
			source["asset"]["config"]["size"]["y"] = count;
		}
		return source;
	}

	[[nodiscard]] Value document(const Value &source)
	{
		Value result = Value::object();
		result["textures"] = Value::object();
		result["textures"]["albedo"]["Texture"] = source;
		return result;
	}

	[[nodiscard]] Value spriteUV(unsigned int x, unsigned int y)
	{
		Value result = Value::object();
		result["sprite"]["x"] = x;
		result["sprite"]["y"] = y;
		return result;
	}

	[[nodiscard]] Value explicitUV(double x, double y, double width, double height)
	{
		Value result = Value::object();
		result["anchor"]["x"] = x;
		result["anchor"]["y"] = y;
		result["size"]["x"] = width;
		result["size"]["y"] = height;
		return result;
	}

	[[nodiscard]] spk::Material fromJSON(spk::Assets &assets, const Value &json)
	{
		spk::Material::Builder builder;
		builder.bind(&assets);
		builder.load(json);
		return builder.build();
	}

	void expectSameBinding(const spk::Material &left, const spk::Material &right, const std::string &role)
	{
		ASSERT_NE(left.find(role), nullptr);
		ASSERT_NE(right.find(role), nullptr);
		EXPECT_EQ(left.find(role)->texture.identifier(), right.find(role)->texture.identifier());
		EXPECT_EQ(left.find(role)->section, right.find(role)->section);
	}
}

TEST(MaterialTest, EmptyMaterialAndFreeNamedRoles)
{
	spk::Material empty;
	EXPECT_TRUE(empty.textures().empty());
	EXPECT_EQ(empty.find("albedo"), nullptr);

	spk::Texture texture;
	spk::Material::Builder builder;
	builder.add("normal", texture);
	builder.add("Normal", texture);
	const spk::Material material = builder.build();
	EXPECT_EQ(material.textures().size(), 2u);
	EXPECT_EQ(material.find("albedo"), nullptr);
	EXPECT_EQ(material.find("normal")->section, spk::Texture::Section::whole);
	EXPECT_EQ(material.find("Normal")->texture.identifier(), texture.handle().identifier());
}

TEST(MaterialTest, SnapshotAndCopiesRemainImmutable)
{
	spk::Texture texture;
	spk::Material::Builder builder;
	builder.add("albedo", texture);
	const spk::Material before = builder.build();
	const spk::Material copy = before;
	builder.add("normal", texture);
	const spk::Material after = builder.build();

	EXPECT_EQ(before.find("normal"), nullptr);
	EXPECT_EQ(copy.find("normal"), nullptr);
	ASSERT_NE(after.find("normal"), nullptr);
	EXPECT_EQ(copy.find("albedo")->texture.identifier(), before.find("albedo")->texture.identifier());
}

TEST(MaterialTest, SharedHandlesSurviveTextureWrappers)
{
	spk::Material first;
	spk::Material second;
	spk::Texture::Handle retained;
	{
		spk::Image texture;
		retained = texture.handle();
		spk::Material::Builder left;
		spk::Material::Builder right;
		left.add("albedo", texture);
		right.add("normal", texture);
		first = left.build();
		second = right.build();
	}
	EXPECT_TRUE(retained.valid());
	EXPECT_EQ(first.find("albedo")->texture.identifier(), retained.identifier());
	EXPECT_EQ(second.find("normal")->texture.identifier(), retained.identifier());
}

TEST(MaterialTest, ExplicitUVAndInvalidManualBindings)
{
	spk::Texture texture;
	spk::Material::Builder builder;
	const spk::Texture::Section region({0.25f, 0.5f}, {0.125f, 0.25f});
	builder.add("albedo", texture, region);
	EXPECT_EQ(builder.build().find("albedo")->section, region);
	EXPECT_THROW(builder.add("albedo", texture), spk::Exception);
	EXPECT_THROW(builder.add("", texture), spk::Exception);
	EXPECT_THROW(builder.add(" \t", texture), spk::Exception);
	EXPECT_THROW(builder.add("zero", texture, {{0, 0}, {0, 1}}), spk::Exception);
	EXPECT_THROW(builder.add("overflow", texture, {{0.9f, 0}, {0.2f, 1}}), spk::Exception);
	EXPECT_THROW(
		builder.add("infinite", texture, {{0, 0}, {std::numeric_limits<float>::infinity(), 1}}),
		spk::Exception);
	EXPECT_EQ(builder.build().textures().size(), 1u);
	EXPECT_NO_THROW(builder.add("edge", texture, {{0.2f, 0.3f}, {0.8f, 0.7f}}));
}

TEST(MaterialTest, JsonAndManualBindingsHaveSameTextureAndUV)
{
	spk::Assets assets;
	spk::Texture &texture = assets.load<spk::Texture>(texturePath());
	spk::SpriteSheet &sheet = assets.load<spk::SpriteSheet>(
		texturePath(), Value::fromString(R"({"size":{"x":16,"y":16}})"));

	spk::Material::Builder manual;
	manual.add("normal", texture);
	manual.add("albedo", sheet, {3, 5});

	Value json = document(pathSource(true));
	json["textures"]["albedo"]["UV"] = spriteUV(3, 5);
	json["textures"]["normal"]["Texture"] = pathSource();

	const spk::Material parsed = fromJSON(assets, json);
	const spk::Material expected = manual.build();
	expectSameBinding(parsed, expected, "albedo");
	expectSameBinding(parsed, expected, "normal");
	EXPECT_EQ(parsed.textures().size(), 2u);
}

TEST(MaterialTest, FileAndTypedSourcesResolveIdenticalTriples)
{
	spk::Assets assets;
	const spk::Material plainPath = fromJSON(assets, document(pathSource()));
	const spk::Material plainAsset = fromJSON(assets, document(assetSource()));
	expectSameBinding(plainPath, plainAsset, "albedo");

	Value atlasPath = document(pathSource(true));
	atlasPath["textures"]["albedo"]["UV"] = spriteUV(3, 5);
	Value atlasAsset = document(assetSource(true));
	atlasAsset["textures"]["albedo"]["UV"] = spriteUV(3, 5);
	expectSameBinding(fromJSON(assets, atlasPath), fromJSON(assets, atlasAsset), "albedo");
}

TEST(MaterialTest, OneImageSupportsIndependentSpriteGrids)
{
	spk::Assets assets;
	const Value sixteen = Value::fromString(R"({"size":{"x":16,"y":16}})");
	const Value eight = Value::fromString(R"({"size":{"x":8,"y":8}})");
	auto &sheet16 = assets.load<spk::SpriteSheet>(texturePath(), sixteen);
	auto &sheet8 = assets.load<spk::SpriteSheet>(texturePath(), eight);
	EXPECT_NE(&sheet16, &sheet8);
	EXPECT_EQ(sheet16.nbSprite(), (spk::Vector2UInt{16, 16}));
	EXPECT_EQ(sheet8.nbSprite(), (spk::Vector2UInt{8, 8}));
	EXPECT_EQ(&assets.get<spk::SpriteSheet>(texturePath()), &sheet16);
	assets.setDefault<spk::SpriteSheet>(texturePath(), eight);
	EXPECT_EQ(&assets.get<spk::SpriteSheet>(texturePath()), &sheet8);

	Value json16 = document(pathSource(true, 16));
	Value json8 = document(pathSource(true, 8));
	json16["textures"]["albedo"]["UV"] = spriteUV(3, 5);
	json8["textures"]["albedo"]["UV"] = spriteUV(3, 5);
	const spk::Material material16 = fromJSON(assets, json16);
	const spk::Material material8 = fromJSON(assets, json8);
	EXPECT_NE(material16.find("albedo")->texture.identifier(), material8.find("albedo")->texture.identifier());
	EXPECT_EQ(material16.find("albedo")->section, sheet16.sprite({3, 5}));
	EXPECT_EQ(material8.find("albedo")->section, sheet8.sprite({3, 5}));
}

TEST(MaterialTest, ExplicitAndWholeUVWorkWithBothKindsOfSources)
{
	spk::Assets assets;
	Value plain = document(pathSource());
	Value atlas = document(pathSource(true));
	EXPECT_EQ(fromJSON(assets, plain).find("albedo")->section, spk::Texture::Section::whole);
	EXPECT_EQ(fromJSON(assets, atlas).find("albedo")->section, spk::Texture::Section::whole);

	plain["textures"]["albedo"]["UV"] = explicitUV(0.25, 0.5, 0.25, 0.25);
	atlas["textures"]["albedo"]["UV"] = explicitUV(0.25, 0.5, 0.25, 0.25);
	EXPECT_EQ(fromJSON(assets, plain).find("albedo")->section, fromJSON(assets, atlas).find("albedo")->section);
	plain["textures"]["albedo"]["UV"] = explicitUV(0.2, 0.3, 0.8, 0.7);
	EXPECT_NO_THROW((void)fromJSON(assets, plain));
}

TEST(MaterialTest, InvalidJSONStructureAndUnknownFieldsAreRejected)
{
	spk::Assets assets;
	const Value empty = Value::fromString(R"({"textures":{}})");
	EXPECT_TRUE(fromJSON(assets, empty).textures().empty());
	EXPECT_THROW(fromJSON(assets, Value{}), spk::Exception);
	EXPECT_THROW(fromJSON(assets, Value::object()), spk::Exception);
	EXPECT_THROW(fromJSON(assets, Value::fromString(R"({"textures":[]})")), spk::Exception);
	EXPECT_THROW(fromJSON(assets, Value::fromString(R"({"textures":{},"extra":1})")), spk::Exception);
	EXPECT_THROW(fromJSON(assets, Value::fromString(R"({"textures":{" ":{"Texture":{"path":"x"}}}})")), spk::Exception);

	Value bad = document(pathSource());
	bad["textures"]["albedo"]["unexpected"] = true;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource());
	bad["textures"]["albedo"]["Texture"]["unknown"] = true;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource());
	bad["textures"]["albedo"]["Texture"]["asset"]["type"] = "Texture";
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
}

TEST(MaterialTest, InvalidAssetSourceAndConfigurationsAreRejected)
{
	spk::Assets assets;
	Value bad = document(assetSource());
	bad["textures"]["albedo"]["Texture"]["size"]["x"] = 16;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);

	bad = document(assetSource());
	bad["textures"]["albedo"]["Texture"]["asset"]["type"] = "EngineView";
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(assetSource());
	bad["textures"]["albedo"]["Texture"]["asset"]["config"] = nullptr;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource(true));
	bad["textures"]["albedo"]["Texture"]["size"]["x"] = 0;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource(true));
	bad["textures"]["albedo"]["Texture"]["size"]["x"] = 1.5;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource());
	bad["textures"]["albedo"]["Texture"]["path"] = "";
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
}

TEST(MaterialTest, InvalidUVAndSpriteCoordinatesAreRejectedWithPaths)
{
	spk::Assets assets;
	Value bad = document(pathSource(true));
	bad["textures"]["albedo"]["UV"] = spriteUV(16, 5);
	try
	{
		(void)fromJSON(assets, bad);
		FAIL() << "Out-of-grid sprite was accepted";
	} catch (const spk::Exception &error)
	{
		EXPECT_NE(std::string(error.what()).find("textures.albedo.UV.sprite"), std::string::npos);
	}

	bad = document(pathSource());
	bad["textures"]["albedo"]["UV"] = spriteUV(0, 0);
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource(true));
	bad["textures"]["albedo"]["UV"] = spriteUV(0, 0);
	bad["textures"]["albedo"]["UV"]["anchor"]["x"] = 0.0;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad = document(pathSource());
	bad["textures"]["albedo"]["UV"] = Value::object();
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad["textures"]["albedo"]["UV"] = nullptr;
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad["textures"]["albedo"]["UV"] = explicitUV(0, 0, 0, 1);
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad["textures"]["albedo"]["UV"] = explicitUV(0.8, 0, 0.4, 1);
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad["textures"]["albedo"]["UV"] = explicitUV(-0.1, 0, 0.5, 1);
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
	bad["textures"]["albedo"]["UV"] = explicitUV(0, 0, 0.5, 0.5);
	bad["textures"]["albedo"]["UV"]["size"]["x"] = "wrong";
	EXPECT_THROW(fromJSON(assets, bad), spk::Exception);
}

TEST(MaterialTest, ParserRejectsDuplicateRoleAndNestedKeys)
{
	EXPECT_THROW(
		(void)Value::fromString(R"({"textures":{"albedo":{},"albedo":{}}})"),
		std::runtime_error);
	EXPECT_THROW(
		(void)Value::fromString(R"({"textures":{"albedo":{"Texture":{"path":"a","path":"b"}}}})"),
		std::runtime_error);
	EXPECT_THROW((void)Value::fromString(R"({"textures":)"), std::runtime_error);
}

TEST(MaterialTest, FailedLoadDoesNotChangeExistingBuilder)
{
	spk::Assets assets;
	spk::Texture existing;
	spk::Material::Builder builder;
	builder.bind(&assets);
	builder.add("existing", existing);
	Value documentWithFailure = document(pathSource());
	documentWithFailure["textures"]["zInvalid"]["Texture"] = pathSource(true);
	documentWithFailure["textures"]["zInvalid"]["UV"] = spriteUV(16, 0);

	EXPECT_THROW(builder.load(documentWithFailure), spk::Exception);
	const spk::Material after = builder.build();
	EXPECT_EQ(after.textures().size(), 1u);
	EXPECT_EQ(after.find("albedo"), nullptr);
	ASSERT_NE(after.find("existing"), nullptr);
	EXPECT_EQ(after.find("existing")->texture.identifier(), existing.handle().identifier());
	EXPECT_NO_THROW(builder.load(document(pathSource())));
	EXPECT_EQ(builder.build().textures().size(), 2u);
	EXPECT_THROW(builder.load(document(pathSource())), spk::Exception);
	EXPECT_EQ(builder.build().textures().size(), 2u);
}

TEST(MaterialTest, BindingRequiresExplicitValidManager)
{
	spk::Material::Builder builder;
	EXPECT_THROW(builder.bind(nullptr), spk::Exception);
	EXPECT_THROW(builder.load(Value::fromString(R"({"textures":{}})")), spk::Exception);
	spk::Assets one;
	spk::Assets two;
	builder.bind(&one);
	EXPECT_THROW(builder.bind(&two), spk::Exception);
	EXPECT_NO_THROW(builder.load(Value::fromString(R"({"textures":{}})")));
}

TEST(MaterialTest, CachedMaterialKeepsHandleAfterManagerClear)
{
	spk::Assets assets;
	Value configuration = document(pathSource());
	spk::Material retained = assets.load<spk::Material>("terrain", configuration);
	const auto identity = retained.find("albedo")->texture.identifier();
	EXPECT_EQ(&assets.get<spk::Material>("terrain"), &assets.load<spk::Material>("terrain", configuration));
	assets.clear();
	EXPECT_EQ(retained.find("albedo")->texture.identifier(), identity);
	EXPECT_TRUE(retained.find("albedo")->texture.valid());
	EXPECT_THROW(assets.get<spk::Material>("terrain"), spk::Exception);
}

TEST(MaterialTest, CPUConstructionRequiresNoActiveOpenGLContext)
{
	spk::Assets assets;
	spk::Texture texture;
	spk::Material::Builder builder;
	builder.add("albedo", texture);
	EXPECT_TRUE(builder.build().find("albedo")->texture.valid());
	EXPECT_TRUE(fromJSON(assets, document(pathSource())).find("albedo")->texture.valid());
}
