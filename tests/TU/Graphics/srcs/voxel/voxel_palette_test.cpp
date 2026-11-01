#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <type_traits>
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
	using VoxelBuilder = spk::Voxel::Palette::Builder;
	using Palette = spk::Palette;
	using Material = spk::Material;
	using Texture = spk::Texture;
	using MeshVertex = spk::Voxel::MeshVertex;

	static_assert(std::is_same_v<decltype(std::declval<VoxelBuilder>().build()), spk::Palette>);

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

	[[nodiscard]] Palette palette(std::initializer_list<std::pair<std::string, Material>> materials)
	{
		VoxelBuilder builder;
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
	EXPECT_EQ(result.materials().size(), 2u);
	EXPECT_FLOAT_EQ(source.materialSlots()[1].opacity, 0.3f);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, ModelDetectsMissingRequiredMaterial)
{
	Texture atlas;
	const auto result = palette({{"stone", material(atlas)}});
	const Model source = model({{"stone", 1.0f}, {"glass", 0.3f}});
	EXPECT_EQ(result.materials().size(), 1u);
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
	VoxelBuilder builder;
	EXPECT_THROW(builder.setMaterial("stone", materialBuilder.build()), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_TRUE(builder.build().materials().empty());
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

TEST(VoxelPaletteTest, RejectsDifferentAtlasesWithoutModel)
{
	Texture first;
	Texture second;
	ASSERT_NE(first.handle().identifier(), second.handle().identifier());
	VoxelBuilder builder;
	builder.setMaterial("left", material(first));
	EXPECT_THROW(builder.setMaterial("right", material(second)), spk::Exception);
	EXPECT_EQ(builder.build().materials().size(), 1u);
}

TEST(VoxelPaletteTest, RejectsDifferentAtlasesRegardlessOfModelOpacity)
{
	Texture first;
	Texture second;
	const Model source = model({{"a", 0.2f}, {"b", 0.7f}});
	VoxelBuilder builder;
	builder.setMaterial("a", material(first));
	EXPECT_THROW(builder.setMaterial("b", material(second)), spk::Exception);
	EXPECT_FALSE(source.isCompatible(builder.build()));
}

TEST(VoxelPaletteTest, RejectsDifferentAtlasesAcrossOpaqueAndTranslucentSlots)
{
	Texture opaque;
	Texture translucent;
	const Model source = model({{"solid", 1.0f}, {"glass", 0.5f}});
	VoxelBuilder builder;
	builder.setMaterial("solid", material(opaque));
	EXPECT_THROW(builder.setMaterial("glass", material(translucent)), spk::Exception);
	EXPECT_FALSE(source.isCompatible(builder.build()));
}

TEST(VoxelPaletteTest, AdditionalPaletteSlotsShareAtlasAndAreIgnoredByModel)
{
	const Model first = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 1.0f}, {"glass", 1.0f}, {"missing", 1.0f}});
	Texture atlas;
	const auto result = palette({{"stone", material(atlas)}, {"glass", material(atlas)}});
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_FALSE(other.isCompatible(result));
}

TEST(VoxelPaletteTest, FailedSetKeepsPreviousPaletteAndBuilder)
{
	Texture first;
	Texture second;
	VoxelBuilder builder;
	builder.setMaterial("stone", material(first));
	const auto original = builder.build();
	EXPECT_THROW(builder.setMaterial("glass", material(second)), spk::Exception);
	EXPECT_EQ(original.materials().size(), 1u);
	EXPECT_EQ(builder.build().materials().size(), 1u);
	builder.setMaterial("glass", material(first));
	const auto repaired = builder.build();
	EXPECT_EQ(repaired.materials().size(), 2u);
}

TEST(VoxelPaletteTest, ReplacingOnlyMaterialMayChangeAtlas)
{
	Texture first;
	Texture second;
	VoxelBuilder builder;
	builder.setMaterial("stone", material(first));
	const auto original = builder.build();
	builder.setMaterial("stone", material(second));
	const auto replacement = builder.build();
	EXPECT_NE(original.find("stone")->find("albedo")->texture.identifier(), replacement.find("stone")->find("albedo")->texture.identifier());
}

TEST(VoxelPaletteTest, DifferentTexturesAreAllowedInGenericPalette)
{
	Texture first;
	Texture second;
	spk::Palette::Builder builder;
	builder.setMaterial("stone", material(first));
	builder.setMaterial("glass", material(second));
	EXPECT_EQ(builder.build().materials().size(), 2u);
}

TEST(VoxelPaletteTest, BuilderFromExistingPalettePreservesAtlasIdentity)
{
	Texture first;
	Texture second;
	const auto initial = palette({{"stone", material(first)}});
	VoxelBuilder variant(initial);
	EXPECT_THROW(variant.setMaterial("glass", material(second)), spk::Exception);
	variant.setMaterial("glass", material(first));
	const auto modified = variant.build();
	EXPECT_EQ(modified.materials().size(), 2u);
	EXPECT_EQ(initial.materials().size(), 1u);
}

TEST(VoxelPaletteTest, EmptyBuilderAllowsEitherFirstAtlas)
{
	Texture first;
	Texture second;
	VoxelBuilder builder;
	EXPECT_TRUE(builder.build().materials().empty());
	builder.setMaterial("stone", material(second));
	EXPECT_THROW(builder.setMaterial("glass", material(first)), spk::Exception);
	EXPECT_EQ(builder.build().find("stone")->find("albedo")->texture.identifier(), second.handle().identifier());
}

TEST(VoxelPaletteTest, SamePaletteWorksWithDifferentModelOpacities)
{
	Texture atlas;
	const auto result = palette({{"shared", material(atlas)}, {"extra", material(atlas)}});
	const Model opaque = model({{"shared", 1.0f}});
	const Model translucent = model({{"shared", 0.4f}, {"extra", 0.2f}});
	EXPECT_TRUE(opaque.isCompatible(result));
	EXPECT_TRUE(translucent.isCompatible(result));
	EXPECT_EQ(result.materials().size(), 2u);
}

TEST(VoxelPaletteTest, OnePaletteWorksWithSeveralModels)
{
	Texture atlas;
	const auto result = palette({{"shared", material(atlas)}, {"left", material(atlas)}, {"right", material(atlas)}});
	const Model first = model({{"shared", 1.0f}, {"left", 0.5f}});
	const Model second = model({{"shared", 1.0f}, {"right", 0.6f}});
	EXPECT_EQ(result.materials().size(), 3u);
	EXPECT_TRUE(first.isCompatible(result));
	EXPECT_TRUE(second.isCompatible(result));
}

TEST(VoxelPaletteTest, BuildsVariantFromExistingPalette)
{
	const Model source = model({{"stone", 1.0f}});
	Texture original;
	Texture replacement;
	const auto before = palette({{"stone", material(original)}});
	VoxelBuilder builder(before);
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
	EXPECT_EQ(&original.materials(), &copy.materials());
	VoxelBuilder builder(original);
	builder.setMaterial("glass", material(atlas));
	const auto expanded = builder.build();
	EXPECT_EQ(original.materials().size(), 1u);
	EXPECT_EQ(copy.materials().size(), 1u);
	EXPECT_EQ(expanded.materials().size(), 2u);
	EXPECT_NE(&expanded.materials(), &original.materials());
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
	VoxelBuilder builder;
	builder.setMaterial("stone", material(atlas));
	EXPECT_THROW(builder.setMaterial("glass", Material{}), spk::Exception);
	EXPECT_THROW(builder.setMaterial("stone", Material{}), spk::Exception);
	EXPECT_THROW(builder.setMaterial("", material(atlas)), spk::Exception);
	EXPECT_EQ(builder.build().materials().size(), 1u);
	builder.setMaterial("glass", material(atlas));
	const Model source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	const auto result = builder.build();
	EXPECT_EQ(result.materials().size(), 2u);
	EXPECT_TRUE(source.isCompatible(result));
}

TEST(VoxelPaletteTest, MissingSlotIsIncompatibleButOpacityMayDiffer)
{
	const Model source = model({{"stone", 1.0f}});
	const Model other = model({{"stone", 0.3f}});
	Texture atlas;
	EXPECT_FALSE(source.isCompatible(Palette{}));
	EXPECT_TRUE(source.isCompatible(palette({{"stone", material(atlas)}})));
}

TEST(VoxelPaletteTest, EmptyModelAcceptsEmptyPalette)
{
	const auto result = VoxelBuilder().build();
	const Model source = model({});
	EXPECT_TRUE(source.isCompatible(result));
	EXPECT_TRUE(result.materials().empty());
}

TEST(VoxelPaletteTest, CanAddMoreSlotsAfterBuild)
{
	Texture atlas;
	VoxelBuilder builder;
	builder.setMaterial("stone", material(atlas));
	const auto first = builder.build();
	builder.setMaterial("glass", material(atlas));
	const auto second = builder.build();
	const Model source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	EXPECT_FALSE(source.isCompatible(first));
	EXPECT_TRUE(source.isCompatible(second));
}

TEST(VoxelPaletteTest, ReplacingOneOfSeveralMaterialsMustKeepSharedAtlas)
{
	Texture first;
	Texture second;
	VoxelBuilder builder;
	builder.setMaterial("stone", material(first));
	builder.setMaterial("glass", material(first));
	EXPECT_THROW(builder.setMaterial("stone", material(second)), spk::Exception);
	EXPECT_EQ(builder.build().find("stone")->find("albedo")->texture.identifier(), first.handle().identifier());
}

TEST(VoxelPaletteTest, RejectsInvalidExistingGenericPalette)
{
	Texture first;
	Texture second;
	spk::Palette::Builder generic;
	generic.setMaterial("stone", material(first));
	generic.setMaterial("glass", material(second));
	EXPECT_THROW((void)VoxelBuilder(generic.build()), spk::Exception);
}

TEST(PaletteTest, OptionalValidatorReceivesKeyMaterialAndExistingEntries)
{
	Texture atlas;
	std::size_t calls = 0;
	spk::Palette::Builder builder(
		[&calls](std::string_view name, const Material &entry, const spk::Palette::Materials &existing) {
			(void)entry;
			EXPECT_EQ(name, "stone");
			EXPECT_TRUE(existing.empty());
			++calls;
		});
	builder.setMaterial("stone", material(atlas));
	EXPECT_EQ(calls, 1u);
	EXPECT_EQ(builder.build().materials().size(), 1u);
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

TEST(VoxelPaletteTest, ModelStartsWithoutPaletteAndCanBindCompatiblePalette)
{
	Texture atlas;
	auto source = model({{"stone", 1.0f}});
	EXPECT_EQ(source.palette(), nullptr);
	const auto appearance = palette({{"stone", material(atlas)}});
	source.bind(appearance);
	ASSERT_NE(source.palette(), nullptr);
	EXPECT_EQ(&source.palette()->materials(), &appearance.materials());
}

TEST(VoxelPaletteTest, ModelBindRejectsMissingRequiredSlotWithoutChangingBinding)
{
	Texture atlas;
	auto source = model({{"stone", 1.0f}, {"glass", 0.5f}});
	const auto complete = palette({{"stone", material(atlas)}, {"glass", material(atlas)}});
	const auto missing = palette({{"stone", material(atlas)}});
	EXPECT_THROW(source.bind(missing), spk::Exception);
	EXPECT_EQ(source.palette(), nullptr);
	source.bind(complete);
	EXPECT_THROW(source.bind(missing), spk::Exception);
	ASSERT_NE(source.palette(), nullptr);
	EXPECT_EQ(&source.palette()->materials(), &complete.materials());
}

TEST(VoxelPaletteTest, ModelBindReportsEveryMissingMaterialSlot)
{
	Texture atlas;
	auto source = model({{"alpha", 1.0f}, {"beta", 0.5f}, {"gamma", 1.0f}});
	VoxelBuilder builder;
	builder.setMaterial("alpha", material(atlas));
	builder.setMaterial("beta", material(atlas));
	builder.setMaterial("gamma", material(atlas));
	const auto complete = builder.build();
	const auto incomplete = palette({{"beta", material(atlas)}});
	source.bind(complete);

	try
	{
		source.bind(incomplete);
		FAIL() << "Expected missing material slots to be rejected";
	} catch (const spk::Exception &error)
	{
		EXPECT_EQ(error.message(), "Voxel Model: palette is missing material slots: 'alpha', 'gamma'");
	}
	ASSERT_NE(source.palette(), nullptr);
	EXPECT_EQ(&source.palette()->materials(), &complete.materials());
}

TEST(VoxelPaletteTest, ModelCopiesCanBindDifferentPalettesWhileSharingGeometry)
{
	Texture first;
	Texture second;
	auto source = bakedSingleCube();
	source.bind(palette({{"stone", material(first)}}));
	auto variant = source;
	variant.bind(palette({{"stone", material(second)}}));
	EXPECT_EQ(&source.opaqueMesh(), &variant.opaqueMesh());
	EXPECT_EQ(&source.depthMesh(), &variant.depthMesh());
	ASSERT_NE(source.palette(), nullptr);
	ASSERT_NE(variant.palette(), nullptr);
	const auto originalAtlas = source.palette()->find("stone")->find("albedo")->texture.identifier();
	const auto variantAtlas = variant.palette()->find("stone")->find("albedo")->texture.identifier();
	EXPECT_NE(originalAtlas, variantAtlas);
}

TEST(VoxelPaletteTest, ModelBindingDoesNotMutateBakedMeshes)
{
	Texture first;
	Texture second;
	auto source = bakedSingleCube();
	const auto before = vertices(source.opaqueMesh());
	const auto beforeIndices = indices(source.opaqueMesh());
	source.bind(palette({{"stone", material(first)}}));
	source.bind(palette({{"stone", material(second)}}));
	const auto after = vertices(source.opaqueMesh());
	ASSERT_EQ(before.size(), after.size());
	for (std::size_t i = 0; i < before.size(); ++i)
	{
		EXPECT_EQ(before[i].position, after[i].position);
		EXPECT_EQ(before[i].normal, after[i].normal);
		EXPECT_EQ(before[i].uv, after[i].uv);
		EXPECT_EQ(before[i].materialIndex, after[i].materialIndex);
	}
	EXPECT_EQ(indices(source.opaqueMesh()), beforeIndices);
}

TEST(VoxelPaletteTest, ModelBindRequiresFullyOccludedSlots)
{
	Texture atlas;
	auto source = bakedOccludedCube();
	EXPECT_THROW(source.bind(palette({{"outside", material(atlas)}})), spk::Exception);
	source.bind(palette({{"outside", material(atlas)}, {"hidden", material(atlas)}}));
	EXPECT_NE(source.palette(), nullptr);
}
