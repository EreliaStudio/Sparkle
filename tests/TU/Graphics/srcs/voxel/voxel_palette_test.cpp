#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "graphics/palette.hpp"
#include "voxel/palette.hpp"
#include "voxel/voxel_mesher.hpp"

namespace
{
	using Model = spk::Voxel::Model;
	using Definition = spk::Voxel::Definition;
	using VoxelPalette = spk::Voxel::Palette;
	using Material = spk::Material;
	using Texture = spk::Texture;
	using MeshVertex = spk::Voxel::MeshVertex;

	[[nodiscard]] Model model(std::initializer_list<Definition::Slot> slots)
	{
		return Model({}, {}, {}, std::vector<Definition::Slot>(slots));
	}

	[[nodiscard]] Material material(
		const Texture &texture,
		Texture::Section section = Texture::Section::whole)
	{
		Material::Builder builder;
		builder.add("albedo", texture, section);
		return builder.build();
	}

	[[nodiscard]] VoxelPalette palette(std::initializer_list<std::pair<std::string, Material>> materials)
	{
		VoxelPalette::Builder builder;
		for (const auto &[name, value] : materials)
		{
			builder.setMaterial(name, value);
		}
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Shape cube()
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		return builder.build();
	}

	[[nodiscard]] Definition definition(const spk::Voxel::Shape &shape, std::string name)
	{
		Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, Definition::Slot{name, 1.0f});
		}
		return Definition(shape, std::move(mapping));
	}

	[[nodiscard]] Model bakedSingleCube()
	{
		Definition::Registry::Builder definitions;
		definitions.add(1, definition(cube(), "stone"));
		spk::Voxel::Volume::Builder volume({1, 1, 1});
		volume.bind(definitions.build());
		volume.set({0, 0, 0}, 1);
		return spk::Voxel::VoxelMesher().bake(volume.build());
	}

	[[nodiscard]] Model bakedOccludedCube()
	{
		const auto shape = cube();
		Definition::Registry::Builder definitions;
		definitions.add(1, definition(shape, "hidden"));
		definitions.add(2, definition(shape, "outside"));
		spk::Voxel::Volume::Builder volume({3, 3, 3});
		volume.bind(definitions.build());
		for (unsigned z = 0; z < 3; ++z)
		{
			for (unsigned y = 0; y < 3; ++y)
			{
				for (unsigned x = 0; x < 3; ++x)
				{
					volume.set({x, y, z}, (x == 1 && y == 1 && z == 1) ? 1 : 2);
				}
			}
		}
		return spk::Voxel::VoxelMesher().bake(volume.build());
	}

	[[nodiscard]] std::vector<MeshVertex> vertices(const spk::Voxel::Mesh &mesh)
	{
		const auto values = mesh.layout().vertexBuffer().cast<MeshVertex>();
		return {values.begin(), values.end()};
	}

	[[nodiscard]] std::vector<std::uint32_t> indices(const spk::Voxel::Mesh &mesh)
	{
		const auto values = mesh.layout().indexBuffer().cast<std::uint32_t>();
		return {values.begin(), values.end()};
	}
}

TEST(VoxelPaletteTest, BuildsPaletteBeforeModelExists)
{
	Texture atlas;
	const auto result = palette({{"stone", material(atlas)}, {"glass", material(atlas)}});
	const Model source = model({{"stone", 1.0f}, {"glass", 0.3f}});
	EXPECT_EQ(result.materials().materials().size(), 2u);
	EXPECT_FLOAT_EQ(source.materialSlots()[1].opacity, 0.3f);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, ModelDetectsMissingRequiredMaterial)
{
	Texture atlas;
	const auto result = palette({{"stone", material(atlas)}});
	const Model source = model({{"stone", 1.0f}, {"glass", 0.3f}});
	EXPECT_EQ(result.materials().materials().size(), 1u);
	EXPECT_FALSE(source.isCompatible(result));
	EXPECT_TRUE(source.isCompatible(
		palette({{"stone", material(atlas)}, {"glass", material(atlas)}})));
}

TEST(VoxelPaletteTest, ModelChecksFullyOccludedSlot)
{
	const Model source = bakedOccludedCube();
	ASSERT_EQ(source.materialSlots().size(), 2u);
	const auto slot = std::find_if(
		source.materialSlots().begin(),
		source.materialSlots().end(),
		[](const auto &entry) {
			return entry.name == "hidden";
		});
	ASSERT_NE(slot, source.materialSlots().end());
	const auto hiddenIndex = static_cast<std::uint32_t>(slot - source.materialSlots().begin());
	for (const auto &vertex : vertices(source.opaqueMesh()))
	{
		EXPECT_NE(vertex.materialIndex, hiddenIndex);
	}
	Texture atlas;
	const auto missing = palette({{"outside", material(atlas)}});
	EXPECT_FALSE(source.isCompatible(missing));
	const auto complete = palette({{"outside", material(atlas)}, {"hidden", material(atlas)}});
	EXPECT_TRUE(source.isCompatible(complete));
}

TEST(VoxelPaletteTest, RejectsMaterialWithoutAlbedoAtSetTime)
{
	Texture atlas;
	Material::Builder materialBuilder;
	materialBuilder.add("normal", atlas);
	VoxelPalette::Builder builder;
	EXPECT_THROW(builder.setMaterial("stone", materialBuilder.build()), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_TRUE(builder.build().materials().materials().empty());
}

TEST(VoxelPaletteTest, AcceptsDifferentUVRegionsOfSameAtlas)
{
	const Model source = model({{"left", 1.0f}, {"right", 1.0f}});
	Texture atlas;
	const auto first = material(atlas, {{0.0f, 0.0f}, {0.5f, 1.0f}});
	const auto second = material(atlas, {{0.5f, 0.0f}, {0.5f, 1.0f}});
	const auto result = palette({{"left", first}, {"right", second}});
	EXPECT_TRUE(source.isCompatible(result));
	EXPECT_EQ(result.find("left")->find("albedo")->texture.identifier(), result.find("right")->find("albedo")->texture.identifier());
	EXPECT_NE(result.find("left")->find("albedo")->section, result.find("right")->find("albedo")->section);
}

TEST(VoxelPaletteTest, RejectsDistinctOpaqueAtlases)
{
	const Model source = model({{"left", 1.0f}, {"right", 1.0f}});
	Texture first;
	Texture second;
	ASSERT_NE(first.handle().identifier(), second.handle().identifier());
	const auto result = palette({{"left", material(first)}, {"right", material(second)}});
	EXPECT_FALSE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, RejectsDistinctTranslucentAtlases)
{
	const Model source = model({{"a", 0.2f}, {"b", 0.7f}});
	Texture first;
	Texture second;
	ASSERT_NE(first.handle().identifier(), second.handle().identifier());
	EXPECT_FALSE(source.isCompatible(
		palette({{"a", material(first)}, {"b", material(second)}})));
}

TEST(VoxelPaletteTest, AllowsDifferentAtlasPerPass)
{
	const Model source = model({{"solid", 1.0f}, {"glass", 0.5f}});
	Texture opaque;
	Texture translucent;
	EXPECT_NE(opaque.handle().identifier(), translucent.handle().identifier());
	EXPECT_TRUE(source.isCompatible(palette({{"solid", material(opaque)}, {"glass", material(translucent)}})));
}

TEST(VoxelPaletteTest, IgnoresAdditionalPaletteSlots)
{
	const Model first = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 1.0f}, {"glass", 1.0f}});
	Texture atlas;
	Texture otherAtlas;
	const auto result = palette({{"stone", material(atlas)}, {"glass", material(otherAtlas)}});
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_FALSE(other.isCompatible(result));
}

TEST(VoxelPaletteTest, SamePaletteWorksWithDifferentModelOpacities)
{
	Texture atlas;
	const auto result = palette({{"shared", material(atlas)}, {"extra", material(atlas)}});
	const Model opaque = model({{"shared", 1.0f}});
	const Model translucent = model({{"shared", 0.4f}, {"extra", 0.2f}});
	EXPECT_TRUE(opaque.isCompatible(result));
	EXPECT_TRUE(translucent.isCompatible(result));
	EXPECT_EQ(result.materials().materials().size(), 2u);
}

TEST(VoxelPaletteTest, OnePaletteWorksWithSeveralModels)
{
	Texture atlas;
	const auto result = palette({
		{"shared", material(atlas)},
		{"left", material(atlas)},
		{"right", material(atlas)}});
	const Model first = model({{"shared", 1.0f}, {"left", 0.5f}});
	const Model second = model({{"shared", 1.0f}, {"right", 0.6f}});
	EXPECT_EQ(result.materials().materials().size(), 3u);
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_TRUE(second.isCompatible(result));
}

TEST(VoxelPaletteTest, BuildsVariantFromExistingPalette)
{
	const Model source = model({{"stone", 1.0f}});
	Texture original;
	Texture replacement;
	const auto before = palette({{"stone", material(original)}});
	VoxelPalette::Builder builder(before);
	builder.setMaterial("stone", material(replacement));
	const auto after = builder.build();
	EXPECT_EQ(before.find("stone")->find("albedo")->texture.identifier(), original.handle().identifier());
	EXPECT_EQ(after.find("stone")->find("albedo")->texture.identifier(), replacement.handle().identifier());
	EXPECT_TRUE(source.isCompatible(before));
	EXPECT_TRUE(source.isCompatible(after));
}

TEST(VoxelPaletteTest, CopiesShareImmutableData)
{
	Texture atlas;
	const auto original = palette({{"stone", material(atlas)}});
	const auto copy = original;
	EXPECT_EQ(&original.materials().materials(), &copy.materials().materials());
	VoxelPalette::Builder builder(original);
	builder.setMaterial("glass", material(atlas));
	const auto expanded = builder.build();
	EXPECT_EQ(original.materials().materials().size(), 1u);
	EXPECT_EQ(copy.materials().materials().size(), 1u);
	EXPECT_EQ(expanded.materials().materials().size(), 2u);
	EXPECT_NE(&expanded.materials().materials(), &original.materials().materials());
}

TEST(VoxelPaletteTest, ChangingPaletteNeverMutatesBakedGeometry)
{
	const Model source = bakedSingleCube();
	ASSERT_FALSE(source.opaqueMesh().empty());
	const auto beforeVertices = vertices(source.opaqueMesh());
	const auto beforeIndices = indices(source.opaqueMesh());
	const auto depthVerticesView = source.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto depthIndicesView = source.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
	const std::vector<spk::Vector3> beforeDepthVertices{depthVerticesView.begin(), depthVerticesView.end()};
	const std::vector<std::uint32_t> beforeDepthIndices{depthIndicesView.begin(), depthIndicesView.end()};
	const auto beforeTransparentVertices = vertices(source.transparentMesh());
	const auto beforeTransparentIndices = indices(source.transparentMesh());
	const auto &originalMesh = source.opaqueMesh();
	Texture first;
	Texture second;
	const auto original = palette({{"stone", material(first)}});
	const auto replacement = palette({{"stone", material(second)}});
	EXPECT_TRUE(source.isCompatible(original));
	EXPECT_TRUE(source.isCompatible(replacement));
	EXPECT_EQ(&source.opaqueMesh(), &originalMesh);
	EXPECT_EQ(indices(source.opaqueMesh()), beforeIndices);
	EXPECT_EQ(vertices(source.opaqueMesh()).size(), beforeVertices.size());
	const auto afterVertices = vertices(source.opaqueMesh());
	for (std::size_t i = 0; i < beforeVertices.size(); ++i)
	{
		EXPECT_EQ(afterVertices[i].position, beforeVertices[i].position);
		EXPECT_EQ(afterVertices[i].normal, beforeVertices[i].normal);
		EXPECT_EQ(afterVertices[i].uv, beforeVertices[i].uv);
		EXPECT_EQ(afterVertices[i].materialIndex, beforeVertices[i].materialIndex);
	}
	EXPECT_EQ(indices(source.transparentMesh()), beforeTransparentIndices);
	EXPECT_EQ(vertices(source.transparentMesh()).size(), beforeTransparentVertices.size());
	const auto depthVerticesAfter = source.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto depthIndicesAfter = source.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
	EXPECT_EQ((std::vector<spk::Vector3>{depthVerticesAfter.begin(), depthVerticesAfter.end()}), beforeDepthVertices);
	EXPECT_EQ((std::vector<std::uint32_t>{depthIndicesAfter.begin(), depthIndicesAfter.end()}), beforeDepthIndices);
}

TEST(VoxelPaletteTest, FailedSetPreservesUsableBuilder)
{
	Texture atlas;
	VoxelPalette::Builder builder;
	builder.setMaterial("stone", material(atlas));
	EXPECT_THROW(builder.setMaterial("glass", Material{}), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_THROW(builder.setMaterial("", material(atlas)), spk::Exception);
	EXPECT_EQ(builder.build().materials().materials().size(), 1u);
	builder.setMaterial("glass", material(atlas));
	const Model source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	const auto result = builder.build();
	EXPECT_EQ(result.materials().materials().size(), 2u);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, MissingSlotIsIncompatibleButOpacityMayDiffer)
{
	const Model source = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 0.3f}});
	Texture atlas;
	EXPECT_FALSE(source.isCompatible(VoxelPalette{}));
	EXPECT_TRUE(source.isCompatible(palette({{"stone", material(atlas)}})));
}

TEST(VoxelPaletteTest, EmptyModelAcceptsEmptyPalette)
{
	const auto result = VoxelPalette::Builder().build();
	const Model source = model({});
	EXPECT_TRUE(source.isCompatible(result));
	EXPECT_TRUE(result.materials().materials().empty());
}

TEST(VoxelPaletteTest, CanAddMoreSlotsAfterBuild)
{
	Texture atlas;
	VoxelPalette::Builder builder;
	builder.setMaterial("stone", material(atlas));
	const auto first = builder.build();
	builder.setMaterial("glass", material(atlas));
	const auto second = builder.build();
	const Model source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	EXPECT_FALSE(source.isCompatible(first));
	EXPECT_TRUE(source.isCompatible(second));
}

TEST(PaletteTest, GenericPaletteAcceptsMaterialsWithoutAlbedo)
{
	spk::Palette::Builder builder;
	spk::Material::Builder materialBuilder;
	Texture atlas;
	materialBuilder.add("normal", atlas);
	const auto normalOnly = materialBuilder.build();
	builder.setMaterial("surface", normalOnly);
	const auto palette = builder.build();
	ASSERT_NE(palette.find("surface"), nullptr);
	EXPECT_EQ(palette.find("surface")->find("albedo"), nullptr);
	EXPECT_NE(palette.find("surface")->find("normal"), nullptr);
	EXPECT_THROW(builder.setMaterial("   ", normalOnly), spk::Exception);
}

TEST(PaletteTest, GenericPaletteSupportsVariantsAndCheapImmutableCopies)
{
	Texture original;
	Texture replacement;
	spk::Palette::Builder builder;
	builder.setMaterial("stone", material(original));
	const auto first = builder.build();
	const auto copy = first;
	spk::Palette::Builder variant(first);
	variant.setMaterial("stone", material(replacement));
	const auto second = variant.build();
	EXPECT_EQ(&first.materials(), &copy.materials());
	EXPECT_NE(&first.materials(), &second.materials());
	EXPECT_EQ(first.find("stone")->find("albedo")->texture.identifier(), original.handle().identifier());
	EXPECT_EQ(second.find("stone")->find("albedo")->texture.identifier(), replacement.handle().identifier());
}
