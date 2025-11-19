// a_star.cpp (updated: respect oneway & drivable ways; nearest-node helper)

#include "a_star.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cmath>
#include <limits>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>

struct Node {
    double lat, lon;
};
struct Edge {
    int64_t to;
    double weight;
};

static std::unordered_map<int64_t, Node> nodes;
static std::unordered_set<int64_t> valid_road_nodes;
static std::unordered_map<int64_t, std::vector<Edge>> adj;
static bool mapLoaded = false;

constexpr double PI_CONST = 3.14159265358979323846;
inline double deg2rad(double deg) { return deg * PI_CONST / 180.0; }

double haversine(double lat1, double lon1, double lat2, double lon2) {
    // Returns distance in meters
    const double R = 6371000.0; // mean Earth radius in meters
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(deg2rad(lat1)) * std::cos(deg2rad(lat2)) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

// Helper: find nearest node id for a lat/lon (linear search - slow for full map, but fine for testing)
int64_t findNearestNode(double lat, double lon) {
    double bestDist = std::numeric_limits<double>::infinity();
    int64_t bestId = 0;
    
    // Only search nodes that are part of the road network
    if (!valid_road_nodes.empty()) {
        for (int64_t id : valid_road_nodes) {
            if (!nodes.count(id)) continue;
            const auto& node = nodes[id];
            double d = haversine(lat, lon, node.lat, node.lon);
            if (d < bestDist) {
                bestDist = d;
                bestId = id;
            }
        }
    } else {
        // Fallback if set is empty (shouldn't happen if map loaded)
        for (const auto &p : nodes) {
            double d = haversine(lat, lon, p.second.lat, p.second.lon);
            if (d < bestDist) {
                bestDist = d;
                bestId = p.first;
            }
        }
    }
    return bestId;
}

void loadKarachiMap(const std::string& filename) {
    struct MapHandler : public osmium::handler::Handler {
        // set of highway tags that are appropriate for motor vehicle routing
        const std::unordered_set<std::string> drivables = {
            "motorway","trunk","primary","secondary","tertiary",
            "unclassified","residential","service","living_street",
            "motorway_link","primary_link","secondary_link","tertiary_link"
        };

        // disallow these (pedestrian/cycle) types explicitly
        const std::unordered_set<std::string> nondrivable = {
            "footway","path","cycleway","steps","pedestrian","track","bridleway","corridor"
        };

        void node(const osmium::Node& node) {
            if (node.location().valid()) {
                nodes[node.id()] = {node.location().lat(), node.location().lon()};
            }
        }

        void way(const osmium::Way& way) {
            const char* highway_tag = way.tags()["highway"];
            if (!highway_tag) return; // not a highway/road-type way

            std::string hw = highway_tag;
            if (nondrivable.count(hw)) return; // skip pedestrian / cycle / steps etc.

            // allow ways that are in drivables set; if not present, skip to be conservative
            if (!drivables.count(hw)) {
                // there are some ambiguous 'road' ways; to be conservative, skip unknown kinds
                return;
            }

            // check simple access restrictions
            const char* access_tag = way.tags()["access"];
            const char* motor_tag = way.tags()["motor_vehicle"];
            if ((access_tag && std::string(access_tag) == "no") ||
                (motor_tag && std::string(motor_tag) == "no")) {
                return; // not allowed for motor vehicles
            }

            // determine one-way behavior
            bool oneway = false;
            bool oneway_reverse = false;
            const char* oneway_tag = way.tags()["oneway"];
            const char* junction_tag = way.tags()["junction"];
            if (junction_tag && std::string(junction_tag) == "roundabout") {
                oneway = true;
            }
            if (oneway_tag) {
                std::string ow(oneway_tag);
                if (ow == "yes" || ow == "true" || ow == "1") oneway = true;
                else if (ow == "-1") oneway_reverse = true;
            }

            const osmium::WayNodeList& wnl = way.nodes();
            // add edges according to the directionality indicated by tags
            for (auto it = wnl.begin(); std::next(it) != wnl.end(); ++it) {
                int64_t id1 = it->ref();
                int64_t id2 = std::next(it)->ref();
                if (!nodes.count(id1) || !nodes.count(id2)) continue; // skip if coordinates unknown

                double d = haversine(nodes[id1].lat, nodes[id1].lon,
                                     nodes[id2].lat, nodes[id2].lon);

                // Add nodes to valid set
                valid_road_nodes.insert(id1);
                valid_road_nodes.insert(id2);

                if (oneway_reverse) {
                    // edge only from id2 -> id1
                    adj[id2].push_back({id1, d});
                } else if (oneway) {
                    // edge only from id1 -> id2 (way node order)
                    adj[id1].push_back({id2, d});
                } else {
                    // bidirectional (normal two-way street)
                    adj[id1].push_back({id2, d});
                    adj[id2].push_back({id1, d});
                }
            }
        }
    };

    try {
        osmium::io::Reader reader(filename);
        MapHandler handler;
        osmium::apply(reader, handler);
        reader.close();
        // Map loaded successfully
    } catch (const std::exception& e) {
        std::cerr << "Error reading Karachi map: " << e.what() << "\n";
    }
}

static std::vector<int64_t> astar(int64_t start, int64_t goal) {
    std::unordered_map<int64_t, double> gScore;
    std::unordered_map<int64_t, double> fScore;
    std::unordered_map<int64_t, int64_t> parent;

    gScore[start] = 0.0;
    fScore[start] = haversine(nodes[start].lat, nodes[start].lon,
                              nodes[goal].lat, nodes[goal].lon);

    auto cmp = [](const std::pair<int64_t, double>& a, const std::pair<int64_t, double>& b) {
        return a.second > b.second;
    };
    std::priority_queue<std::pair<int64_t, double>,
                       std::vector<std::pair<int64_t, double>>,
                       decltype(cmp)> openSet(cmp);

    openSet.push({start, fScore[start]});

    int nodes_explored = 0;

    while (!openSet.empty()) {
        auto current_pair = openSet.top();
        openSet.pop();
        int64_t current = current_pair.first;
        double current_fscore_in_queue = current_pair.second;

        if (fScore.count(current) && current_fscore_in_queue > fScore[current] + 1e-9) {
            continue; // stale entry
        }

        nodes_explored++;

        if (current == goal) {
            std::vector<int64_t> path;
            for (int64_t at = goal; at != start; at = parent[at]) {
                path.push_back(at);
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (!adj.count(current)) continue;

        for (const auto& edge : adj[current]) {
            double tentative_gScore = gScore[current] + edge.weight;

            if (!gScore.count(edge.to) || tentative_gScore < gScore[edge.to]) {
                parent[edge.to] = current;
                gScore[edge.to] = tentative_gScore;
                fScore[edge.to] = tentative_gScore +
                    haversine(nodes[edge.to].lat, nodes[edge.to].lon,
                              nodes[goal].lat, nodes[goal].lon);

                openSet.push({edge.to, fScore[edge.to]});
            }
        }
    }

    return {};
}

// Public API functions

void initAStar(const std::string& mapFile) {
    if (!mapLoaded) {
        loadKarachiMap(mapFile);
        mapLoaded = true;
    }
}

PathResult aStarWithNodes(int64_t startNode, int64_t endNode) {
    PathResult result;
    result.found = false;

    if (!nodes.count(startNode) || !nodes.count(endNode)) {
        std::cerr << "Invalid node IDs (not found in loaded OSM nodes).\n";
        return result;
    }

    // Check if nodes have outgoing edges (are part of the road network)
    if (!adj.count(startNode)) {
        std::cerr << "Warning: Start node " << startNode << " exists but has no outgoing edges (not part of drivable road network).\n";
    }
    if (!adj.count(endNode)) {
        std::cerr << "Warning: End node " << endNode << " exists but has no outgoing edges (not part of drivable road network).\n";
    }

    std::vector<int64_t> path = astar(startNode, endNode);
    if (!path.empty()) {
        result.nodeIds = path;
        result.found = true;
    } else {
        // Provide helpful diagnostics
        if (!adj.count(startNode) || !adj.count(endNode)) {
            std::cerr << "Path not found: One or both nodes are not part of the drivable road network.\n";
        } else {
            std::cerr << "Path not found: No route exists between these nodes (they may be in disconnected parts of the road network).\n";
        }
    }

    return result;
}

PathResult aStarWithCoords(double startLat, double startLon, double endLat, double endLon) {
    PathResult result;
    result.found = false;

    int64_t start = findNearestNode(startLat, startLon);
    int64_t goal = findNearestNode(endLat, endLon);

    if (!nodes.count(start) || !nodes.count(goal)) {
        std::cerr << "Could not find valid nodes near given coordinates.\n";
        return result;
    }

    // Check if nearest nodes have outgoing edges (are part of the road network)
    if (!adj.count(start)) {
        std::cerr << "Warning: Nearest start node " << start << " exists but has no outgoing edges (not part of drivable road network).\n";
        std::cerr << "  Try coordinates closer to a drivable road.\n";
    }
    if (!adj.count(goal)) {
        std::cerr << "Warning: Nearest end node " << goal << " exists but has no outgoing edges (not part of drivable road network).\n";
        std::cerr << "  Try coordinates closer to a drivable road.\n";
    }

    std::vector<int64_t> path = astar(start, goal);
    if (!path.empty()) {
        result.nodeIds = path;
        result.found = true;
    } else {
        // Provide helpful diagnostics
        if (!adj.count(start) || !adj.count(goal)) {
            std::cerr << "Path not found: One or both nearest nodes are not part of the drivable road network.\n";
            std::cerr << "  The nearest nodes to your coordinates may be on non-drivable paths (footways, etc.).\n";
        } else {
            std::cerr << "Path not found: No route exists between these nodes (they may be in disconnected parts of the road network).\n";
        }
    }

    return result;
}

bool getNodeCoords(int64_t nodeId, double& lat, double& lon) {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        lat = it->second.lat;
        lon = it->second.lon;
        return true;
    }
    return false;
}

void convertPathToVertices(const std::vector<int64_t>& pathNodeIds,
                          float midX, float midY, float scale,
                          std::vector<float>& outVertices,
                          std::vector<unsigned int>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    if (pathNodeIds.empty() || nodes.empty()) {
        return;
    }

    const double deg2rad = M_PI / 180.0;

    // Convert each node in path to vertices
    for (size_t i = 0; i < pathNodeIds.size(); ++i) {
        double lat, lon;
        if (!getNodeCoords(pathNodeIds[i], lat, lon)) {
            continue; // Skip invalid nodes
        }

        // Convert to Web Mercator (same as map_data.cpp)
        double lon_rad = lon * deg2rad;
        double lat_rad = lat * deg2rad;
        double x_merc = lon_rad;
        double y_merc = 0.5 * std::log((1.0 + std::sin(lat_rad)) / (1.0 - std::sin(lat_rad)));

        // Normalize to [-1, 1] (same as map_data.cpp)
        float nx = (static_cast<float>(x_merc) - midX) * (2.0f / scale);
        float ny = (static_cast<float>(y_merc) - midY) * (2.0f / scale);
        float z = 0.0f;

        outVertices.push_back(nx);
        outVertices.push_back(ny);
        outVertices.push_back(z);

        // Add index for line strip
        outIndices.push_back(static_cast<unsigned int>(i));
    }
}