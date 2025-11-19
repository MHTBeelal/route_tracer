#include "map_data.hpp"

// Map_Data.cpp (modified to include node lat/lon output and snapping)

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/way.hpp>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <cstdint>

struct Road {
    std::string name;
    std::string type;
    std::vector<std::vector<osmium::object_id_type>> segments; // Each "Way" is one segment
};

class MyHandler : public osmium::handler::Handler {
public:
    std::map<std::pair<std::string, std::string>, Road> mergedRoads;
    std::unordered_map<osmium::object_id_type, std::pair<double, double>> node_coords;

    void node(const osmium::Node& node) {
        if (node.location().valid()) {
            node_coords[node.id()] = { node.location().lat(), node.location().lon() };
        }
    }

    void way(const osmium::Way& way) {
        const char* highway = way.tags()["highway"];
        const char* name = way.tags()["name"];

        static const std::unordered_set<std::string> major_roads = {
            "motorway", "trunk", "primary", "secondary", "tertiary",
            "unclassified", "residential", "service", "living_street",
            "motorway_link", "primary_link", "secondary_link", "tertiary_link"
        };

        if (highway && major_roads.count(highway)) {
            std::pair<std::string, std::string> key(name ? name : "unnamed", highway);

            std::vector<osmium::object_id_type> nodes;
            for (const auto& node_ref : way.nodes()) {
                nodes.push_back(node_ref.ref());
            }

            auto& road = mergedRoads[key];
            if (road.name.empty()) {
                road.name = name ? name : "unnamed";
                road.type = highway;
            }
            road.segments.push_back(nodes);
        }
    }

    void printMergedData(std::ostream& out) const {
        for (const auto& entry : mergedRoads) {
            const auto& road = entry.second;

            out << "Road: " << road.name
                << " | Type: " << road.type
                << " | Segments: " << road.segments.size()
                << "\n";

            for (size_t i = 0; i < road.segments.size(); ++i) {
                const auto& seg = road.segments[i];
                if (!seg.empty()) {
                    out << "  Segment " << (i + 1)
                        << " → Nodes: " << seg.front()
                        << " ... " << seg.back()
                        << " (" << seg.size() << " nodes)\n";

                    auto print_coord = [&](osmium::object_id_type nid) {
                        auto it = node_coords.find(nid);
                        if (it != node_coords.end()) {
                            out << "     Node " << nid << " [lat: " << std::fixed << std::setprecision(7)
                                << it->second.first << ", lon: " << it->second.second << "]\n";
                        } else {
                            out << "     Node " << nid << " [lat/lon: unknown]\n";
                        }
                    };

                    // show first node coords
                    print_coord(seg.front());
                    // if more than 1 node, show last node coords
                    if (seg.size() > 1) {
                        print_coord(seg.back());
                    }

                    // (optional) show up to first 3 intermediate nodes' coords to help debugging
                    size_t show_count = std::min<size_t>(3, seg.size());
                    if (seg.size() > 2) {
                        out << "     Sample intermediate nodes:\n";
                        for (size_t k = 1; k <= show_count && k + 1 < seg.size(); ++k) {
                            osmium::object_id_type nid = seg[k];
                            auto it = node_coords.find(nid);
                            if (it != node_coords.end()) {
                                out << "       " << nid << " [lat: " << std::fixed << std::setprecision(7)
                                    << it->second.first << ", lon: " << it->second.second << "]\n";
                            } else {
                                out << "       " << nid << " [lat/lon: unknown]\n";
                            }
                        }
                    }
                }
            }
            out << "------------------------------------\n";
        }
    }
};

Map parseMap(const std::string& filepath) {
    const std::string input_file = "res/data/karachi.osm.pbf";
    Map out;

    try {
        osmium::io::Reader reader(input_file);
        MyHandler handler;

        osmium::apply(reader, handler);
        reader.close();

        if (handler.node_coords.empty()) {
            std::cerr << "No node coordinates parsed from map file.\n";
            return out;
        }

        // Compute bounding box
        double minLat = std::numeric_limits<double>::max();
        double maxLat = std::numeric_limits<double>::lowest();
        double minLon = std::numeric_limits<double>::max();
        double maxLon = std::numeric_limits<double>::lowest();

        for (const auto& kv : handler.node_coords) {
            double lat = kv.second.first;
            double lon = kv.second.second;
            minLat = std::min(minLat, lat);
            maxLat = std::max(maxLat, lat);
            minLon = std::min(minLon, lon);
            maxLon = std::max(maxLon, lon);
        }

        double latRange = (maxLat - minLat);
        double lonRange = (maxLon - minLon);
        if (latRange == 0) latRange = 1.0;
        if (lonRange == 0) lonRange = 1.0;

        // Map node id -> vertex index
        std::unordered_map<osmium::object_id_type, unsigned int> node_index;

        // --- snapping helper to merge vertices very close to each other ---
        const double SNAP_EPS = 1e-7; // tune if needed
        std::unordered_map<uint64_t, unsigned int> spatial_index;
        auto make_key = [&](double x, double y) -> uint64_t {
            int64_t xi = static_cast<int64_t>(std::llround(x / SNAP_EPS));
            int64_t yi = static_cast<int64_t>(std::llround(y / SNAP_EPS));
            uint64_t ux = static_cast<uint64_t>(xi) & 0xffffffffULL;
            uint64_t uy = static_cast<uint64_t>(yi) & 0xffffffffULL;
            return (ux << 32) | uy;
        };

        // Build vertices and indices (line segments)
        for (const auto& entry : handler.mergedRoads) {
            const auto& road = entry.second;
            for (const auto& seg : road.segments) {
                if (seg.size() < 2) continue;

                for (size_t i = 0; i < seg.size(); ++i) {
                    osmium::object_id_type nid = seg[i];
                    auto it = node_index.find(nid);
                    if (it == node_index.end()) {
                        auto coordIt = handler.node_coords.find(nid);
                        if (coordIt == handler.node_coords.end()) continue; // skip unknown nodes

                        double lat = coordIt->second.first;
                        double lon = coordIt->second.second;

                        // Project to Web Mercator for better visual layout
                        const double deg2rad = M_PI / 180.0;
                        double lon_rad = lon * deg2rad;
                        double lat_rad = lat * deg2rad;

                        // Web Mercator projection (x = lon, y = ln(tan(pi/4 + lat/2)))
                        double x_merc = lon_rad;
                        double y_merc = 0.5 * std::log((1.0 + std::sin(lat_rad)) / (1.0 - std::sin(lat_rad)));

                        float z = 0.0f;

                        // Spatial snapping: merge coords that fall into same snap cell
                        uint64_t skey = make_key(x_merc, y_merc);
                        auto sit = spatial_index.find(skey);
                        unsigned int idx;
                        if (sit != spatial_index.end()) {
                            idx = sit->second;
                        } else {
                            idx = static_cast<unsigned int>(out.vertices.size() / 3);
                            out.vertices.push_back(static_cast<float>(x_merc));
                            out.vertices.push_back(static_cast<float>(y_merc));
                            out.vertices.push_back(z);
                            spatial_index[skey] = idx;
                        }

                        node_index[nid] = idx;
                    }
                }

                // Add indices for this segment as a contiguous run so we can draw GL_LINE_STRIP per segment
                size_t startOffset = out.indices.size();
                size_t added = 0;
                for (size_t i = 0; i < seg.size(); ++i) {
                    auto itIdx = node_index.find(seg[i]);
                    if (itIdx == node_index.end()) continue;
                    out.indices.push_back(itIdx->second);
                    ++added;
                }
                if (added >= 2) {
                    // compute approx segment extent to filter tiny segments
                    float x0 = out.vertices[out.indices[startOffset] * 3 + 0];
                    float y0 = out.vertices[out.indices[startOffset] * 3 + 1];
                    float x1 = out.vertices[out.indices[startOffset + added - 1] * 3 + 0];
                    float y1 = out.vertices[out.indices[startOffset + added - 1] * 3 + 1];
                    float extent = std::hypot(x1 - x0, y1 - y0);
                    const float MIN_SEG_EXTENT = 1e-6f; // filter threshold (in mercator units)
                    if (extent >= MIN_SEG_EXTENT) {
                        out.segmentOffsets.push_back(startOffset);
                        out.segmentLengths.push_back(added);
                    } else {
                        out.indices.resize(startOffset);
                    }
                } else {
                    // rollback if segment has fewer than 2 valid points
                    out.indices.resize(startOffset);
                }
            }
        }

        std::cout << "Parsed map: vertices=" << (out.vertices.size()/3) << " indices=" << out.indices.size() << "\n";

        // Normalize mercator coordinates to NDC [-1,1] while preserving aspect ratio
        if (!out.vertices.empty()) {
            // collect coordinates
            std::vector<float> xs;
            std::vector<float> ys;
            xs.reserve(out.vertices.size() / 3);
            ys.reserve(out.vertices.size() / 3);
            for (size_t i = 0; i < out.vertices.size(); i += 3) {
                xs.push_back(out.vertices[i]);
                ys.push_back(out.vertices[i+1]);
            }

            // compute robust percentiles (5%-95%) to ignore outliers
            auto percentile = [&](std::vector<float>& v, double p) {
                if (v.empty()) return 0.0f;
                size_t idx = static_cast<size_t>(std::floor(p * (v.size() - 1)));
                std::vector<float> tmp = v;
                std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
                return tmp[idx];
            };

            float x_lo = percentile(xs, 0.05);
            float x_hi = percentile(xs, 0.95);
            float y_lo = percentile(ys, 0.05);
            float y_hi = percentile(ys, 0.95);

            float midX = (x_lo + x_hi) * 0.5f;
            float midY = (y_lo + y_hi) * 0.5f;
            float rangeX = x_hi - x_lo;
            float rangeY = y_hi - y_lo;
            float scale = std::max(rangeX, rangeY);
            if (scale == 0.0f) scale = 1.0f;

            out.midX = midX;
            out.midY = midY;
            out.scale = scale;

            // normalize to [-1,1]
            for (size_t i = 0; i < out.vertices.size(); i += 3) {
                float x = out.vertices[i];
                float y = out.vertices[i+1];
                float nx = (x - midX) * (2.0f / scale);
                float ny = (y - midY) * (2.0f / scale);
                out.vertices[i] = nx;
                out.vertices[i+1] = ny;
            }
        }

        

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
    }

    return out;
}