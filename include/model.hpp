#pragma once

#include <cstdint>
#include <memory>

#include "skeleton.hpp"

namespace spk
{
	class Model
	{
		friend class Skeleton;
		friend class Skeleton::Part;
		friend class Skeleton::Joint;

	public:
		class View;

		struct Version
		{
			std::uint64_t transform = 0;
			std::uint64_t pose = 0;
			std::uint64_t topology = 0;
			std::uint64_t geometry = 0;

			[[nodiscard]] bool operator==(const Version &) const noexcept = default;
		};

	private:
		std::unique_ptr<Skeleton> _skeleton;
		spk::Matrix4x4 _transform = spk::Matrix4x4::identity();
		std::uint64_t _transformVersion = 0;
		mutable spk::CachedData<spk::Matrix4x4> _globalMatrix{[this] {
			return _calculateGlobalMatrix();
		}};

		[[nodiscard]] spk::Matrix4x4 _calculateGlobalMatrix() const;
		void _invalidateGlobalMatrices() noexcept;

	public:
		Model();
		explicit Model(Skeleton skeleton);
		Model(const Model &other);
		Model(Model &&other) noexcept;
		Model &operator=(const Model &other);
		Model &operator=(Model &&other) noexcept;
		~Model();

		[[nodiscard]] Skeleton &skeleton() noexcept;
		[[nodiscard]] const Skeleton &skeleton() const noexcept;
		void setTransform(spk::Matrix4x4 transform);
		[[nodiscard]] const spk::Matrix4x4 &transform() const noexcept;
		[[nodiscard]] const spk::Matrix4x4 &globalMatrix() const;
		[[nodiscard]] Version version() const noexcept;
	};

}
