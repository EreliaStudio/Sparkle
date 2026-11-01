#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
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
		class AnchorPath final
		{
		private:
			std::vector<std::string> _segments;

		public:
			AnchorPath() = default;
			AnchorPath(std::initializer_list<std::string> segments);
			explicit AnchorPath(std::vector<std::string> segments);
			[[nodiscard]] const std::vector<std::string> &segments() const noexcept;
			[[nodiscard]] bool empty() const noexcept;
			[[nodiscard]] std::size_t size() const noexcept;
			[[nodiscard]] bool operator<(const AnchorPath &other) const noexcept;
			[[nodiscard]] bool operator==(const AnchorPath &other) const noexcept;
			[[nodiscard]] std::vector<std::string>::const_iterator begin() const noexcept;
			[[nodiscard]] std::vector<std::string>::const_iterator end() const noexcept;
			void push_back(std::string segment);
			void pop_back();
		};

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
		using Path = AnchorPath;

	public:
		class AnchorHandle;

	private:
		Model _model;
		std::map<Path, spk::Palette> _palettes;
		std::map<Path, spk::Quaternion> _rotations;
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		std::shared_ptr<std::uint8_t> _lifetime = std::make_shared<std::uint8_t>(0);

		[[nodiscard]] const Model::Anchor &_anchor(const Path &path) const;
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
		View(const View &other);
		View(View &&other);
		View &operator=(const View &other);
		View &operator=(View &&other);
		~View() = default;

		[[nodiscard]] AnchorHandle anchor(const AnchorPath &path);
		[[nodiscard]] AnchorHandle anchor(std::string_view uniqueName);
		[[nodiscard]] const Model &model() const noexcept;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] const spk::Palette *palette(std::string_view anchorName) const;
		[[nodiscard]] const spk::Palette *palette(const AnchorPath &path) const;
		void setPalette(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Recursive);
		void setPalette(
			std::string_view anchorName,
			spk::Palette palette,
			PalettePropagation propagation = PalettePropagation::Self);
		void setPalette(
			const AnchorPath &path,
			spk::Palette palette,
			PalettePropagation propagation = PalettePropagation::Self);
		void setAnchorRotation(std::string_view name, spk::Quaternion rotation);
		void setAnchorRotation(const AnchorPath &path, spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion anchorRotation(std::string_view name) const;
		[[nodiscard]] spk::Quaternion anchorRotation(const AnchorPath &path) const;
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] bool isRenderable() const noexcept;
		void visit(const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const;
	};

	class Model::View::AnchorHandle final
	{
	private:
		View *_view;
		AnchorPath _path;
		std::weak_ptr<std::uint8_t> _lifetime;

		explicit AnchorHandle(View &view, AnchorPath path);
		friend class View;

	public:
		[[nodiscard]] const AnchorPath &path() const noexcept;
		void setRotation(spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion rotation() const;
		void resetRotation();
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
