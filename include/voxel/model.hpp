#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/palette.hpp"
#include "math/matrix.hpp"
#include "voxel/geometry.hpp"

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

		class Anchor;

		class View;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

		explicit Model(std::shared_ptr<const Data> data) noexcept;

	public:
		Model();
		Model(spk::Voxel::Geometry geometry);

		[[nodiscard]] const spk::Voxel::Geometry &geometry() const noexcept;
		[[nodiscard]] const spk::Voxel::Geometry::SurfaceMesh &opaqueMesh() const noexcept;
		[[nodiscard]] const spk::Voxel::Geometry::SurfaceMesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;

		[[nodiscard]] bool hasAnchor(std::string_view name) const noexcept;
		[[nodiscard]] const Anchor &anchor(std::string_view name) const;
		[[nodiscard]] const std::map<std::string, Anchor, std::less<>> &anchors() const noexcept;
	};

	class Model::Anchor final
	{
		friend class Builder;

	public:
		class View;

		using Path = std::vector<std::string>;
		static constexpr const char *NoAttachmentPoint = "";

	private:
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		std::optional<Model> _model;
		std::string _attachment;

	public:
		explicit Anchor(spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] const Model *model() const noexcept;
		[[nodiscard]] const std::string &attachment() const noexcept;
	};

	class Model::Builder final
	{
	private:
		spk::Voxel::Geometry _geometry;
		std::map<std::string, Anchor, std::less<>> _anchors;

	public:
		Builder() = default;
		explicit Builder(const Model &source);
		void setGeometry(spk::Voxel::Geometry geometry);
		void addAnchor(std::string name, spk::Matrix4x4 transform = spk::Matrix4x4::identity());
		void removeAnchor(std::string_view name);
		void setAnchorTransform(std::string_view name, spk::Matrix4x4 transform);
		void attach(std::string_view name, const Model &model, std::string attachment = Anchor::NoAttachmentPoint);
		void detach(std::string_view name);
		void harmonize();
		[[nodiscard]] Model build() const;
	};

	class Model::Anchor::View final
	{
	private:
		spk::Quaternion _rotation = spk::Quaternion::identity();
		std::unique_ptr<Model::View> _child;

	public:
		explicit View(const Anchor &anchor);
		View(const View &other);
		View(View &&other) noexcept;
		View &operator=(const View &other);
		View &operator=(View &&other) noexcept;
		~View();

		[[nodiscard]] bool hasChild() const noexcept;
		[[nodiscard]] Model::View &child();
		[[nodiscard]] const Model::View &child() const;
		void setRotation(spk::Quaternion rotation);
		[[nodiscard]] spk::Quaternion rotation() const noexcept;
		void resetRotation() noexcept;
	};

	class Model::View final
	{
	private:
		Model _model;
		std::optional<spk::Palette> _palette;
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		std::map<std::string, Anchor::View, std::less<>> _anchors;

		[[nodiscard]] Anchor::Path _findUnique(std::string_view name) const;
		void _collectMatches(
			std::string_view name,
			Anchor::Path &path,
			std::vector<Anchor::Path> &matches) const;
		void _validatePalette(const spk::Palette &palette, PalettePropagation propagation) const;
		void _applyPalette(const spk::Palette &palette, PalettePropagation propagation);
		[[nodiscard]] spk::Matrix4x4 _childTransform(
			const Anchor &anchor,
			const Anchor::View &view,
			spk::Matrix4x4 parent) const;
		void _visit(
			const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback,
			spk::Matrix4x4 parent) const;

	public:
		explicit View(Model model);
		View(Model model, spk::Palette palette);
		View(const View &other);
		View(View &&other) noexcept;
		View &operator=(const View &other);
		View &operator=(View &&other) noexcept;
		~View();

		[[nodiscard]] const Model &model() const noexcept;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] Anchor::View &anchor(const Anchor::Path &path);
		[[nodiscard]] const Anchor::View &anchor(const Anchor::Path &path) const;
		[[nodiscard]] Anchor::View &anchor(std::string_view uniqueName);
		[[nodiscard]] const Anchor::View &anchor(std::string_view uniqueName) const;
		void bind(spk::Palette palette, PalettePropagation propagation = PalettePropagation::Self);
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] bool isRenderable() const;
		void visit(const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const;
	};

}
