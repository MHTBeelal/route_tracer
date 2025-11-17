#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "map_data.hpp"
#include "a_star.hpp"

#include "windower.hpp"
#include "renderer.hpp"


int main(void)
{  
    // Parse map and provide geometry to renderer
    // aStar();

    Renderer renderer;

    Map map = parseMap("res/data/karachi.osm.pbf");
    
    if (!map.vertices.empty() && !map.indices.empty()) {
        renderer.setVertices(map.vertices);
        renderer.setIndices(map.indices);

        // provide per-segment info so Renderer can draw continuous strips
        renderer.setSegmentInfo(map.segmentOffsets, map.segmentLengths);
        renderer.setDrawMode(GL_LINE_STRIP);
    }

    Windower windower(renderer, 800, 640);
    windower.run();

}