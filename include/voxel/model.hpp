#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/palette.hpp"
#include "math/matrix.hpp"
#include "voxel/material_layout.hpp"
#include "voxel/mesh.hpp"

namespace spk::Voxel
{
	enum class PalettePropagation
	{
		Self,
		Recursive
	};

	class Model
	{
	public:
		class Anchor
		{
		private:
			spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
			std::unique_ptr<Model> _model;
			std::optional<std::string> _attachment;

		public:
			explicit Anchor(spk::Matrix4x4 transform = spk::Matrix4x4::identity());
			Anchor(const Anchor &other);
			Anchor(Anchor &&other) noexcept;
			Anchor &operator=(const Anchor &other);
			Anchor &operator=(Anchor &&other) noexcept;
			~Anchor();

			[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
			void setTransform(spk::Matrix4x4 transform);
			void bind(const Model &model, std::optional<std::string> attachment = std::nullopt);
			void unbind() noexcept;
			[[nodiscard]] Model *model() noexcept;
			[[nodiscard]] const Model *model() const noexcept;
			[[nodiscard]] const std::optional<std::string> &attachment() const noexcept;
		};

		class View;
		class Harmonizer;

	private:
		spk::Voxel::Mesh _mesh;
		std::map<std::string, Anchor, std::less<>> _anchors;

	public:
		Model() = default;
		Model(spk::Voxel::Mesh mesh);
		Model(
			SurfaceMesh opaque,
			SurfaceMesh translucent,
			spk::ShadowMap::Mesh depth,
			std::vector<Definition::Slot> slots,
			std::uint32_t materialIndexOffset = 0);

		[[nodiscard]] const spk::Voxel::Mesh &mesh() const noexcept;
		[[nodiscard]] Model withMesh(spk::Voxel::Mesh mesh) const;
		[[nodiscard]] const SurfaceMesh &opaqueMesh() const noexcept;
		[[nodiscard]] const SurfaceMesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] std::uint32_t materialIndexOffset() const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;

		void addAnchor(std::string name, spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		void removeAnchor(std::string_view name);
		[[nodiscard]] bool hasAnchor(std::string_view name) const noexcept;
		[[nodiscard]] Anchor &anchor(std::string_view name);
		[[nodiscard]] const Anchor &anchor(std::string_view name) const;
		[[nodiscard]] const std::map<std::string, Anchor, std::less<>> &anchors() const noexcept;
	};

	class Model::View final
	{
		friend class Model::Harmonizer;
		using Path = std::vector<std::string>;

	private:
		Model _model;
		std::map<Path, spk::Palette> _palettes;
		std::map<Path, spk::Quaternion> _rotations;
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();

		[[nodiscard]] Path _findUnique(std::string_view name) const;
		[[nodiscard]] const Model &_node(const Path &path) const;
		void _applyPalette(
			Path path,
			const spk::Palette &palette,
			PalettePropagation propagation,
			std::map<Path, spk::Palette> &prepared) const;
		[[nodiscard]] spk::Matrix4x4 _childTransform(
			const Model::Anchor &anchor,
			const Path &path,
			spk::Matrix4x4 parent) const;
		void _visit(
			const Model &model,
			Path &path,
			spk::Matrix4x4 transform,
			const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const;

	public:
		explicit View(Model model);
		View(Model model, spk::Palette palette);

		[[nodiscard]] const Model &model() const noexcept;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] const spk::Palette *palette(std::string_view anchorName) const;
		void setPalette(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Recursive);
		void setPalette(
			std::string_view anchorName,
			spk::Palette palette,
			PalettePropagation propagation = PalettePropagation::Self);
		void setAnchorRotation(std::string_view name, spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion anchorRotation(std::string_view name) const;
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] bool isRenderable() const noexcept;
		void visit(const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const;
	};

	class Model::Harmonizer final
	{
	public:
		struct Result
		{
			Model model;
			spk::Palette palette;
			spk::Voxel::MaterialLayout materialLayout;
		};

	private:
		using Path = View::Path;

		struct Context
		{
			std::vector<std::string> names;
			std::vector<spk::Material> materials;
			std::vector<std::string> origins;
			std::map<Path, std::vector<std::uint32_t>> remappings;
			std::vector<std::string> reservedNames;
		};

		[[nodiscard]] static std::uint32_t _materialIndex(
			const Definition::Slot &slot,
			const spk::Material &material,
			Context &context);
		static void _normalize(Context &context);
		[[nodiscard]] static spk::Palette _buildPalette(const Context &context);
		static void _collect(
			const View &view,
			const Model &model,
			Path &path,
			Context &context);
		static void _rewrite(
			Model &model,
			Path &path,
			const Context &context);

	public:
		[[nodiscard]] Result process(const View &view) const;
	};
}
