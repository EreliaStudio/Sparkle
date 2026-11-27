#pragma once

#include <cstddef>

namespace spk::ShadowMap
{
	// Optional per-execution diagnostic counters. Durations are CPU wall time
	// in milliseconds, including any blocking inside the OpenGL driver.
	struct Profile
	{
		double command = 0;
		double projection = 0;
		double selection = 0;
		double targetAcquire = 0;
		double stateSetup = 0;
		double targetActivate = 0;
		double clear = 0;
		double sourceLookup = 0;
		double geometryPack = 0;
		double drawList = 0;
		double programSetup = 0;
		double transformPrepare = 0;
		double transformActivate = 0;
		double geometryActivate = 0;
		double draw = 0;
		double depthCopy = 0;
		double publication = 0;

		std::size_t targetHits = 0;
		std::size_t targetMisses = 0;
		std::size_t mapsRendered = 0;
		std::size_t geometryPacks = 0;
		std::size_t geometryBytes = 0;
		std::size_t transformUploads = 0;
		std::size_t transformBytes = 0;
		std::size_t transformCacheHits = 0;
		std::size_t drawCalls = 0;
		std::size_t meshDraws = 0;
		std::size_t triangles = 0;
	};
}
