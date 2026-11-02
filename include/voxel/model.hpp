#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/palette.hpp"
#include "math/matrix.hpp"
#include "voxel/material_layout.hpp"
#include "voxel/mesh.hpp"

namespace spk::Voxel
{
	class Model
	{
	public:
		enum class PalettePropagation
		{
			Self,
			Recursive
		};

		class Builder;

		class Anchor final
		{
			friend class Builder;

		public:
			using Path = std::vector<std::string>;
			static constexpr const char *NoAttachmentPoint = "";

		private:
			spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
			std::shared_ptr<const Model> _model;
			std::string _attachment;

		public:
			explicit Anchor(spk::Matrix4x4 transform = spk::Matrix4x4::identity());
			[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
			[[nodiscard]] const Model *model() const noexcept;
			[[nodiscard]] const std::string &attachment() const noexcept;
		};

		class View;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

		explicit Model(std::shared_ptr<const Data> data) noexcept;

	public:
		Model();
		Model(spk::Voxel::Mesh mesh);

		[[nodiscard]] const spk::Voxel::Mesh &mesh() const noexcept;
		[[nodiscard]] const spk::Voxel::Mesh::SurfaceMesh &opaqueMesh() const noexcept;
		[[nodiscard]] const spk::Voxel::Mesh::SurfaceMesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] std::uint32_t materialIndexOffset() const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;

		[[nodiscard]] bool hasAnchor(std::string_view name) const noexcept;
		[[nodiscard]] const Anchor &anchor(std::string_view name) const;
		[[nodiscard]] const std::map<std::string, Anchor, std::less<>> &anchors() const noexcept;
	};

	class Model::Builder final
	{
	private:
		spk::Voxel::Mesh _mesh;
		std::map<std::string, Anchor, std::less<>> _anchors;

	public:
		Builder() = default;
		explicit Builder(const Model &source);
		void setMesh(spk::Voxel::Mesh mesh);
		void addAnchor(std::string name, spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		void removeAnchor(std::string_view name);
		void setAnchorTransform(std::string_view name, spk::Matrix4x4 transform);
		void bind(std::string_view name, const Model &model, std::string attachment = Anchor::NoAttachmentPoint);
		void unbind(std::string_view name);
		void harmonize();
		[[nodiscard]] Model build() const;
	};

	class Model::View final
	{
		using Path = Anchor::Path;

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

		[[nodiscard]] AnchorHandle anchor(const Anchor::Path &path);
		[[nodiscard]] AnchorHandle anchor(std::string_view uniqueName);
		[[nodiscard]] const Model &model() const noexcept;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] const spk::Palette *palette(std::string_view anchorName) const;
		[[nodiscard]] const spk::Palette *palette(const Anchor::Path &path) const;
		void setPalette(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Recursive);
		void setPalette(
			std::string_view anchorName,
			spk::Palette palette,
			PalettePropagation propagation = PalettePropagation::Self);
		void setPalette(
			const Anchor::Path &path,
			spk::Palette palette,
			PalettePropagation propagation = PalettePropagation::Self);
		void setAnchorRotation(std::string_view name, spk::Quaternion rotation);
		void setAnchorRotation(const Anchor::Path &path, spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion anchorRotation(std::string_view name) const;
		[[nodiscard]] spk::Quaternion anchorRotation(const Anchor::Path &path) const;
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] bool isRenderable() const noexcept;
		void visit(const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const;
	};

	class Model::View::AnchorHandle final
	{
	private:
		View *_view;
		Anchor::Path _path;
		std::weak_ptr<std::uint8_t> _lifetime;

		explicit AnchorHandle(View &view, Anchor::Path path);
		friend class View;

	public:
		[[nodiscard]] const Anchor::Path &path() const noexcept;
		void setRotation(spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion rotation() const;
		void resetRotation();
	};
}
