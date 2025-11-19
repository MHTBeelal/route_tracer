#ifndef A_STAR
#define A_STAR

#include <vector>
#include <cstdint>
#include <string>

// Path result structure
struct PathResult {
    std::vector<int64_t> nodeIds;  // Path as sequence of node IDs
    bool found;                     // Whether a path was found
};

// Initialize A* with map data (should be called once at startup)
void initAStar(const std::string& mapFile);

// Run A* pathfinding with node IDs
PathResult aStarWithNodes(int64_t startNode, int64_t endNode);

// Run A* pathfinding with coordinates (finds nearest nodes)
PathResult aStarWithCoords(double startLat, double startLon, double endLat, double endLon);

// Get node coordinates for a node ID (for path conversion)
bool getNodeCoords(int64_t nodeId, double& lat, double& lon);

// Find nearest node ID for a lat/lon
int64_t findNearestNode(double lat, double lon);

// Convert path node IDs to renderable vertices/indices
// Uses the same coordinate transformation as the map (Web Mercator + normalization)
// mapVertices: normalized map vertices from parseMap (used to calculate normalization params)
// Convert path node IDs to renderable vertices/indices
// Uses the same coordinate transformation as the map (Web Mercator + normalization)
void convertPathToVertices(const std::vector<int64_t>& pathNodeIds,
                          float midX, float midY, float scale,
                          std::vector<float>& outVertices,
                          std::vector<unsigned int>& outIndices);

#endif
