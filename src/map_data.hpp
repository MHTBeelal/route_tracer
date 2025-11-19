#ifndef MAP_DATA
#define MAP_DATA

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
#include <chrono>
#include <iomanip>
#include <sstream>

struct Map {
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	std::vector<size_t> segmentOffsets;
	std::vector<size_t> segmentLengths;

	// Normalization parameters
	float midX = 0.0f;
	float midY = 0.0f;
	float scale = 1.0f;
};

Map parseMap(const std::string& filepath);

#endif