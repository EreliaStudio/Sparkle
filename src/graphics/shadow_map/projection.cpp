#include "shadow_map/projection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "exception.hpp"
#include "math/vector4.hpp"

namespace spk::ShadowMap
{
	namespace
	{
		[[nodiscard]] Vector3 normalizedDirection(const Vector3 &direction)
		{
			const double squared = direction.dot(direction);
			if (!std::isfinite(squared) || squared < 1.0e-12)
			{
				throw spk::Exception("Shadow projection requires a finite nonzero light direction");
			}
			return direction.normalized();
		}

		[[nodiscard]] Vector3 upFor(const Vector3 &direction)
		{
			return std::abs(direction.y) < 0.95f ? Vector3{0, 1, 0} : Vector3{1, 0, 0};
		}

		[[nodiscard]] std::array<Vector3, 8> truncatedFrustum(const Camera3D &camera, float distance, float minimumDepth = -std::numeric_limits<float>::infinity())
		{
			if (!std::isfinite(distance) || distance <= 0.0f)
			{
				throw spk::Exception("Directional shadow distance must be finite and positive");
			}
			const Matrix4x4 &projection = camera.projectionMatrix();
			const bool perspective = camera.projectionMode() == Camera3D::ProjectionMode::Perspective;
			float nearPlane;
			if (perspective)
			{
				nearPlane = projection[3][2] / (projection[2][2] - 1.0f);
			}
			else
			{
				nearPlane = (projection[3][2] + 1.0f) / projection[2][2];
			}
			if (!std::isfinite(nearPlane) || !std::isfinite(projection[0][0]) ||
				!std::isfinite(projection[1][1]) || projection[0][0] == 0.0f ||
				projection[1][1] == 0.0f)
			{
				throw spk::Exception("Invalid camera projection for directional shadows");
			}
			const float farPlane = std::max(nearPlane + 0.001f, distance);
			const Matrix4x4 inverseView =
				Matrix4x4::translation(camera.position()) * Matrix4x4::rotation(camera.rotation());
			std::array<Vector3, 8> corners{};
			for (std::size_t i = 0; i < corners.size(); ++i)
			{
				const float x = (i & 1) ? 1.0f : -1.0f;
				const float y = (i & 2) ? 1.0f : -1.0f;
				const float depth = (i & 4) ? farPlane : std::max(nearPlane, minimumDepth);
				const float vx = perspective ? x * depth / projection[0][0] : (x - projection[3][0]) / projection[0][0];
				const float vy = perspective ? y * depth / projection[1][1] : (y - projection[3][1]) / projection[1][1];
				const Vector4 world = inverseView * Vector4{vx, vy, -depth, 1.0f};
				corners[i] = {world.x, world.y, world.z};
			}
			return corners;
		}
	}

	std::array<Projection, 6> Projection::point(const Light::Selection::Entry &light)
	{
		if (light.type != Light::Selection::Type::Point || !std::isfinite(light.range) ||
			light.range <= 0.0f || !std::isfinite(light.position.x) ||
			!std::isfinite(light.position.y) || !std::isfinite(light.position.z))
		{
			throw spk::Exception("Invalid Point shadow projection parameters");
		}
		const float nearPlane = light.range * 0.001f;
		if (!std::isfinite(nearPlane) || nearPlane <= 0.0f ||
			!(nearPlane < light.range))
		{
			throw spk::Exception("Point shadow near/far planes cannot be represented");
		}
		constexpr std::array<Vector3, 6> directions{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
		constexpr std::array<Vector3, 6> ups{{{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}}};
		std::array<Projection, 6> result{};
		for (std::size_t face = 0; face < result.size(); ++face)
		{
			Camera3D camera;
			camera.setPosition(light.position);
			camera.lookAt(light.position + directions[face], ups[face]);
			camera.setPerspective(1.57079632679f, 1.0f, nearPlane, light.range);
			result[face] = {camera.viewProjectionMatrix(), nearPlane, light.range};
		}
		return result;
	}

	Projection Projection::spot(const Light::Selection::Entry &light)
	{
		if (light.type != Light::Selection::Type::Spot ||
			!std::isfinite(light.range) || light.range <= 0.0f ||
			!std::isfinite(light.angle) || light.angle <= 0.0f ||
			light.angle >= 1.57079632679f)
		{
			throw spk::Exception("Invalid Spot shadow projection parameters");
		}
		const Vector3 direction = normalizedDirection(light.direction);
		const float nearPlane = std::min(0.05f, light.range * 0.01f);
		Camera3D camera;
		camera.setPosition(light.position);
		camera.lookAt(light.position + direction, upFor(direction));
		camera.setPerspective(2.0f * light.angle, 1.0f, nearPlane, light.range);
		return {camera.viewProjectionMatrix(), nearPlane, light.range};
	}

	Projection Projection::directional(const Light::Selection::Entry &light, const Camera3D &camera)
	{
		if (light.type != Light::Selection::Type::Directional)
		{
			throw spk::Exception("Directional shadow projection requires a Directional light");
		}
		const Vector3 direction = normalizedDirection(light.direction);
		const auto corners = truncatedFrustum(camera, light.shadowDistance);
		Vector3 center{};
		for (const Vector3 &point : corners)
		{
			center += point;
		}
		center = center / Vector3{8, 8, 8};
		float radius = 0.0f;
		for (const Vector3 &point : corners)
		{
			radius = std::max(radius, static_cast<float>((point - center).length()));
		}
		radius = std::max(radius, 0.01f);
		Camera3D shadowCamera;
		shadowCamera.setPosition(center - direction * Vector3{2.0f * radius, 2.0f * radius, 2.0f * radius});
		shadowCamera.lookAt(center, upFor(direction));
		const Matrix4x4 view = shadowCamera.viewMatrix();
		float minX = std::numeric_limits<float>::max(), minY = minX, minZ = minX;
		float maxX = std::numeric_limits<float>::lowest(), maxY = maxX, maxZ = maxX;
		for (const Vector3 &point : corners)
		{
			const Vector4 projected = view * Vector4{point.x, point.y, point.z, 1.0f};
			minX = std::min(minX, projected.x);
			minY = std::min(minY, projected.y);
			minZ = std::min(minZ, projected.z);
			maxX = std::max(maxX, projected.x);
			maxY = std::max(maxY, projected.y);
			maxZ = std::max(maxZ, projected.z);
		}
		const float padding = std::max(0.01f, radius * 0.01f);
		const float nearPlane = std::max(0.001f, -maxZ - padding);
		const float farPlane = std::max(nearPlane + 0.01f, -minZ + padding);
		const Matrix4x4 orthographic = Matrix4x4::ortho(minX - padding, maxX + padding, minY - padding, maxY + padding, nearPlane, farPlane);
		return {orthographic * view, nearPlane, farPlane};
	}

	std::array<Projection, Projection::CascadeCount> Projection::directionalCascades(
		const Light::Selection::Entry &light, const Camera3D &camera, std::span<const CasterBounds> casters, std::size_t count,
		std::span<const Projection> previousProjections)
	{
		if (light.type != Light::Selection::Type::Directional || light.shadowResolution == 0 || (count != 1 && count != CascadeCount))
		{
			throw spk::Exception("Invalid directional cascade parameters");
		}
		const auto direction = normalizedDirection(light.direction);
		if (!std::isfinite(light.shadowDistance) || light.shadowDistance <= 0.0f)
		{
			throw spk::Exception("Directional shadow distance must be finite and positive");
		}
		const auto &projection = camera.projectionMatrix();
		const bool perspective = camera.projectionMode() == Camera3D::ProjectionMode::Perspective;
		const float nearDepth = perspective ? projection[3][2] / (projection[2][2] - 1.0f) :
											  (projection[3][2] + 1.0f) / projection[2][2];
		const float cameraFar = perspective ? projection[3][2] / (projection[2][2] + 1.0f) :
											  (projection[3][2] - 1.0f) / projection[2][2];
		const float distance = std::isfinite(cameraFar) ? std::min(cameraFar, light.shadowDistance) : light.shadowDistance;
		const float farDepth = std::max(nearDepth + 0.004f, distance);
		Camera3D lightCamera;
		lightCamera.lookAt(direction, upFor(direction));
		const auto view = lightCamera.viewMatrix();

		const auto projectBounds = [](const CasterBounds &box, const Matrix4x4 &matrix) {
			const auto center = (box.minimum + box.maximum) * Vector3{0.5f, 0.5f, 0.5f};
			const auto extent = (box.maximum - box.minimum) * Vector3{0.5f, 0.5f, 0.5f};
			const auto p = matrix * Vector4(center, 1.0f);
			const auto radius = [&](std::size_t row) {
				return std::abs(matrix[0][row]) * extent.x + std::abs(matrix[1][row]) * extent.y +
					   std::abs(matrix[2][row]) * extent.z;
			};
			const Vector3 r{radius(0), radius(1), radius(2)};
			const Vector3 c{p.x, p.y, p.z};
			return CasterBounds{c - r, c + r};
		};
		std::vector<CasterBounds> lightBounds;
		lightBounds.reserve(casters.size());
		for (const auto &caster : casters)
		{
			lightBounds.push_back(projectBounds(caster, view));
		}

		// Fractions of the receiver depth interval; overlap the preceding slice
		// by ten percent of its width for the shader's transition.
		constexpr std::array<float, CascadeCount> fractions{0.125f, 0.28125f, 0.53125f, 1.0f};
		std::array<Projection, CascadeCount> result{};
		float previous = nearDepth;
		float previousWidth = 0.0f;
		for (std::size_t i = 0; i < count; ++i)
		{
			const float split = nearDepth + (farDepth - nearDepth) * (count == 1 ? 1.0f : fractions[i]);
			const auto corners = truncatedFrustum(camera, split, previous - previousWidth * 0.1f);
			Vector3 minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			Vector3 maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
			for (const auto &corner : corners)
			{
				const auto p = view * Vector4(corner, 1.0f);
				minimum = {std::min(minimum.x, p.x), std::min(minimum.y, p.y), std::min(minimum.z, p.z)};
				maximum = {std::max(maximum.x, p.x), std::max(maximum.y, p.y), std::max(maximum.z, p.z)};
			}
			const float extent = std::max(maximum.x - minimum.x, maximum.y - minimum.y);
			const float half = std::ceil(std::max(0.01f, extent * 0.54f) * 64.0f) / 64.0f;
			const float resolution = static_cast<float>(light.shadowResolution);
			const float paddedHalf = resolution >= 8.0f ? half / (1.0f - 4.0f / resolution) : half;
			float texel = 2.0f * paddedHalf / resolution;
			const auto snap = [&](float coordinate) { return resolution >= 8.0f ? std::round(coordinate / texel) * texel : coordinate; };
			const float x = snap((minimum.x + maximum.x) * 0.5f);
			const float y = snap((minimum.y + maximum.y) * 0.5f);
			float left = x - paddedHalf, right = x + paddedHalf;
			float bottom = y - paddedHalf, top = y + paddedHalf;
			// A light-space guard band makes the map independent of small
			// camera translations and rotations. Never reuse a larger map if
			// doing so would materially reduce the requested texel density.
			const Projection *cached = i < previousProjections.size() ? &previousProjections[i] : nullptr;
			bool reuseFootprint = false;
			if (cached && cached->worldPerTexel > 0.0f && cached->worldPerTexel <= texel * 1.10f)
			{
				const auto &coverage = cached->lightSpaceCoverage;
				const float border = 2.0f * cached->worldPerTexel;
				reuseFootprint = minimum.x >= coverage.minimum.x + border &&
					maximum.x <= coverage.maximum.x - border && minimum.y >= coverage.minimum.y + border &&
					maximum.y <= coverage.maximum.y - border;
				if (reuseFootprint)
				{
					left = coverage.minimum.x;
					right = coverage.maximum.x;
					bottom = coverage.minimum.y;
					top = coverage.maximum.y;
					texel = cached->worldPerTexel;
				}
			}
			// Only extend towards the light for casters overlapping the receiver
			// footprint. Off-camera blockers remain in the shadow map.
			for (const auto &box : lightBounds)
			{
				if (box.maximum.x >= left && box.minimum.x <= right &&
					box.maximum.y >= bottom && box.minimum.y <= top && box.maximum.z >= minimum.z)
				{
					maximum.z = std::max(maximum.z, box.maximum.z);
				}
			}
			const float depthPadding = std::max(texel, (right - left) * 0.02f);
			float nearPlane = std::floor((-maximum.z - depthPadding) / texel) * texel;
			float farPlane = std::ceil((-minimum.z + depthPadding) / texel) * texel;
			if (reuseFootprint && cached->nearPlane <= -maximum.z - texel &&
				cached->farPlane >= -minimum.z + texel &&
				cached->farPlane - cached->nearPlane <= 1.5f * (farPlane - nearPlane))
			{
				nearPlane = cached->nearPlane;
				farPlane = cached->farPlane;
			}
			result[i] = {Matrix4x4::ortho(left, right, bottom, top, nearPlane, farPlane) * view,
						 nearPlane, farPlane, split, texel, previous,
						 {{left, bottom, -farPlane}, {right, top, -nearPlane}}};
			previousWidth = split - previous;
			previous = split;
		}
		return result;
	}

	bool Projection::intersects(const CasterBounds &bounds) const
	{
		// Conservative AABB vs. the orthographic directional clip volume.
		const auto center = (bounds.minimum + bounds.maximum) * Vector3{0.5f, 0.5f, 0.5f};
		const auto extent = (bounds.maximum - bounds.minimum) * Vector3{0.5f, 0.5f, 0.5f};
		const auto p = viewProjection * Vector4(center, 1.0f);
		const std::array<float, 3> coordinates{p.x, p.y, p.z};
		for (std::size_t axis = 0; axis < coordinates.size(); ++axis)
		{
			const float radius = std::abs(viewProjection[0][axis]) * extent.x +
								 std::abs(viewProjection[1][axis]) * extent.y +
								 std::abs(viewProjection[2][axis]) * extent.z;
			if (coordinates[axis] - radius > p.w || coordinates[axis] + radius < -p.w)
			{
				return false;
			}
		}
		return true;
	}
}
