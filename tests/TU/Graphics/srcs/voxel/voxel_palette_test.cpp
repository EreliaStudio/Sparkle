#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "graphics/voxel_palette.hpp"
#include "voxel/voxel_mesher.hpp"

namespace
{
	using Model = spk::Voxel::Model;
	using Definition = spk::Voxel::Definition;
	using VoxelPalette = spk::VoxelPalette;
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

	[[nodiscard]] VoxelPalette palette(
		const Model &source,
		std::initializer_list<std::pair<std::string, Material>> materials)
	{
		VoxelPalette::Builder builder(source);
		for (const auto &[name, value] : materials)
		{
			builder.setMaterial(name, value);
		}
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Shape cube()
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(
			std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
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

TEST(VoxelPaletteTest, BuildsCompletePaletteFromModel)
{
	const Model source = model({{"stone", 1.0f}, {"glass", 0.3f}});
	Texture atlas;
	const auto result = palette(source, {{"stone", material(atlas)}, {"glass", material(atlas)}});
	EXPECT_EQ(result.slots().size(), 2u);
	EXPECT_FLOAT_EQ(result.find("glass")->opacity, 0.3f);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, RejectsMissingRequiredMaterial)
{
	const Model source = model({{"stone", 1.0f}, {"glass", 0.3f}});
	Texture atlas;
	VoxelPalette::Builder builder(source);
	builder.setMaterial("stone", material(atlas));
	EXPECT_THROW((void)builder.build(), spk::Exception);
	builder.setMaterial("glass", material(atlas));
	EXPECT_TRUE(source.isCompatible(builder.build()));
}

TEST(VoxelPaletteTest, RequiresMaterialForFullyOccludedSlot)
{
	const Model source = bakedOccludedCube();
	ASSERT_EQ(source.materialSlots().size(), 2u);
	const auto slot = std::find_if(
		source.materialSlots().begin(),
		source.materialSlots().end(),
		[](const auto &entry) { return entry.name == "hidden"; });
	ASSERT_NE(slot, source.materialSlots().end());
	const auto hiddenIndex = static_cast<std::uint32_t>(slot - source.materialSlots().begin());
	for (const auto &vertex : vertices(source.opaqueMesh()))
	{
		EXPECT_NE(vertex.materialIndex, hiddenIndex);
	}
	Texture atlas;
	VoxelPalette::Builder builder(source);
	builder.setMaterial("outside", material(atlas));
	EXPECT_THROW((void)builder.build(), spk::Exception);
	builder.setMaterial("hidden", material(atlas));
	EXPECT_TRUE(source.isCompatible(builder.build()));
}

TEST(VoxelPaletteTest, RejectsMaterialWithoutAlbedo)
{
	const Model source = model({{"stone", 1.0f}});
	Texture atlas;
	Material::Builder materialBuilder;
	materialBuilder.add("normal", atlas);
	VoxelPalette::Builder builder(source);
	EXPECT_THROW(builder.setMaterial("stone", materialBuilder.build()), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_THROW((void)builder.build(), spk::Exception);
}

TEST(VoxelPaletteTest, AcceptsDifferentUVRegionsOfSameAtlas)
{
	const Model source = model({{"left", 1.0f}, {"right", 1.0f}});
	Texture atlas;
	const auto first = material(atlas, {{0.0f, 0.0f}, {0.5f, 1.0f}});
	const auto second = material(atlas, {{0.5f, 0.0f}, {0.5f, 1.0f}});
	const auto result = palette(source, {{"left", first}, {"right", second}});
	EXPECT_TRUE(source.isCompatible(result));
	EXPECT_EQ(result.find("left")->material.find("albedo")->texture.identifier(),
			  result.find("right")->material.find("albedo")->texture.identifier());
	EXPECT_NE(result.find("left")->material.find("albedo")->section,
			  result.find("right")->material.find("albedo")->section);
}

TEST(VoxelPaletteTest, RejectsDistinctOpaqueAtlases)
{
	const Model source = model({{"left", 1.0f}, {"right", 1.0f}});
	Texture first;
	Texture second;
	ASSERT_NE(first.handle().identifier(), second.handle().identifier());
	const auto result = palette(source, {{"left", material(first)}, {"right", material(second)}});
	EXPECT_FALSE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, RejectsDistinctTranslucentAtlases)
{
	const Model source = model({{"a", 0.2f}, {"b", 0.7f}});
	Texture first;
	Texture second;
	ASSERT_NE(first.handle().identifier(), second.handle().identifier());
	EXPECT_FALSE(source.isCompatible(
		palette(source, {{"a", material(first)}, {"b", material(second)}})));
}

TEST(VoxelPaletteTest, AllowsDifferentAtlasPerPass)
{
	const Model source = model({{"solid", 1.0f}, {"glass", 0.5f}});
	Texture opaque;
	Texture translucent;
	EXPECT_NE(opaque.handle().identifier(), translucent.handle().identifier());
	EXPECT_TRUE(source.isCompatible(palette(
		source, {{"solid", material(opaque)}, {"glass", material(translucent)}})));
}

TEST(VoxelPaletteTest, IgnoresAdditionalPaletteSlots)
{
	const Model first = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 1.0f}, {"glass", 1.0f}});
	Texture atlas;
	Texture otherAtlas;
	VoxelPalette::Builder builder(other);
	builder.setMaterial("stone", material(atlas));
	builder.setMaterial("glass", material(otherAtlas));
	const auto result = builder.build();
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_FALSE(other.isCompatible(result));
}

TEST(VoxelPaletteTest, RejectsConflictingModelOpacitiesAtomically)
{
	Texture atlas;
	const Model first = model({{"shared", 1.0f}});
	const Model conflicting = model({{"extra", 0.2f}, {"shared", 0.4f}});
	VoxelPalette::Builder builder(first);
	EXPECT_THROW(builder.addModel(conflicting), spk::Exception);
	builder.setMaterial("shared", material(atlas));
	EXPECT_THROW(builder.setMaterial("extra", material(atlas)), spk::Exception);
	EXPECT_EQ(builder.build().slots().size(), 1u);
}

TEST(VoxelPaletteTest, CombinesModelsWithSharedContracts)
{
	const Model first = model({{"shared", 1.0f}, {"left", 0.5f}});
	const Model second = model({{"shared", 1.0f}, {"right", 0.6f}});
	Texture atlas;
	VoxelPalette::Builder builder(first);
	builder.addModel(second);
	builder.setMaterial("shared", material(atlas));
	builder.setMaterial("left", material(atlas));
	builder.setMaterial("right", material(atlas));
	const auto result = builder.build();
	EXPECT_EQ(result.slots().size(), 3u);
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_TRUE(second.isCompatible(result));
}

TEST(VoxelPaletteTest, BuildsVariantFromExistingPalette)
{
	const Model source = model({{"stone", 1.0f}});
	Texture original;
	Texture replacement;
	const auto before = palette(source, {{"stone", material(original)}});
	VoxelPalette::Builder builder(before);
	builder.setMaterial("stone", material(replacement));
	const auto after = builder.build();
	EXPECT_EQ(before.find("stone")->material.find("albedo")->texture.identifier(), original.handle().identifier());
	EXPECT_EQ(after.find("stone")->material.find("albedo")->texture.identifier(), replacement.handle().identifier());
	EXPECT_TRUE(source.isCompatible(before));
	EXPECT_TRUE(source.isCompatible(after));
}

TEST(VoxelPaletteTest, CopiesShareImmutableData)
{
	const Model source = model({{"stone", 1.0f}});
	Texture atlas;
	const auto original = palette(source, {{"stone", material(atlas)}});
	const auto copy = original;
	EXPECT_EQ(&original.slots(), &copy.slots());
	VoxelPalette::Builder builder(original);
	builder.declareSlot("glass", 0.5f);
	builder.setMaterial("glass", material(atlas));
	const auto expanded = builder.build();
	EXPECT_EQ(original.slots().size(), 1u);
	EXPECT_EQ(copy.slots().size(), 1u);
	EXPECT_EQ(expanded.slots().size(), 2u);
	EXPECT_NE(&expanded.slots(), &original.slots());
}

TEST(VoxelPaletteTest, ChangingPaletteNeverMutatesBakedGeometry)
{
	const Model source = bakedSingleCube();
	ASSERT_FALSE(source.opaqueMesh().empty());
	const auto beforeVertices = vertices(source.opaqueMesh());
	const auto beforeIndices = indices(source.opaqueMesh());
	const auto beforeDepthIndices = indices(source.opaqueMesh());
	const auto &originalMesh = source.opaqueMesh();
	Texture first;
	Texture second;
	const auto original = palette(source, {{"stone", material(first)}});
	const auto replacement = palette(source, {{"stone", material(second)}});
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
	EXPECT_EQ(indices(source.opaqueMesh()), beforeDepthIndices);
}

TEST(VoxelPaletteTest, FailedSetAndBuildPreserveUsableBuilder)
{
	Texture atlas;
	const Model source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	VoxelPalette::Builder builder(source);
	EXPECT_THROW(builder.setMaterial("missing", material(atlas)), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_THROW(builder.declareSlot("", 1.0f), spk::Exception);
	EXPECT_THROW(builder.declareSlot("invalid", std::numeric_limits<float>::infinity()), spk::Exception);
	EXPECT_THROW(builder.declareSlot("stone", 0.4f), spk::Exception);
	builder.setMaterial("stone", material(atlas));
	EXPECT_THROW((void)builder.build(), spk::Exception);
	builder.setMaterial("glass", material(atlas));
	const auto result = builder.build();
	EXPECT_EQ(result.slots().size(), 2u);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, MissingSlotAndOpacityMismatchAreIncompatible)
{
	const Model source = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 0.3f}});
	Texture atlas;
	EXPECT_FALSE(source.isCompatible(VoxelPalette{}));
	EXPECT_FALSE(source.isCompatible(palette(other, {{"stone", material(atlas)}})));
}

TEST(VoxelPaletteTest, EmptyModelAcceptsEmptyPalette)
{
	const Model source = model({});
	const auto result = VoxelPalette::Builder(source).build();
	EXPECT_TRUE(source.isCompatible(result));
	EXPECT_TRUE(result.slots().empty());
}
