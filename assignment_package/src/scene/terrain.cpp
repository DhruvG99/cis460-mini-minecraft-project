#include "terrain.h"
#include "glm/gtc/random.hpp"
#include <stdexcept>
#include <iostream>
#include <chrono>
using namespace std::chrono;
using namespace glm;

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context)
{}

Terrain::~Terrain()
{
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(this->mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = std::move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

#if 1
//TODO: m2: draw chunk border
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram)
{
    // rendering all the opaque cubes
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk!=nullptr)
            {
                chunk->createChunkVBOdata(x, z, false);
                chunk->createVBOdata();
                shaderProgram->drawInter(*chunk);
            }
        }
    }

    // rendering all the transparent cubes
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk!=nullptr)
            {
                chunk->createChunkVBOdata(x, z, true);
                chunk->createVBOdata();
                shaderProgram->drawInter(*chunk);
            }
        }
    }
}
#endif

vec2 random2(vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)))) * 43758.54f);
}
float surflet(vec2 P, vec2 gridPoint) {
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1.0 - 6.0 * pow(distX, 5.0) + 15.0 * pow(distX, 4.0) - 10.0 * pow(distX, 3.0);
    float tY = 1.0 - 6.0 * pow(distY, 5.0) + 15.0 * pow(distY, 4.0) - 10.0 * pow(distY, 3.0);

    vec2 gradient = random2(gridPoint);
    vec2 diff = P - gridPoint;
    float height = dot(diff, gradient);
    return height * tX * tY;
}

float PerlinNoise(vec2 uv) {
    vec2 uvXLYL = floor(uv);
    vec2 uvXHYL = uvXLYL + vec2(1,0);
    vec2 uvXHYH = uvXLYL + vec2(1,1);
    vec2 uvXLYH = uvXLYL + vec2(0,1);
    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
}

float worleyNoise(vec2 uv, float f) {
    uv *= f;
    vec2 uvint = floor(uv);
    vec2 uvfract = fract(uv);
    float minD = 1.f;
    for(int y=-1; y<=1; y++){
        for(int x = -1; x<=1; x++) {
            vec2 neigh = vec2(float(x), float(y));
            vec2 point = random2(uvint + neigh);
            vec2 diff = neigh + point - uvfract;
            float dist = length(diff);
            minD = glm::min(minD, dist);
        }
    }
    return minD;
}

int obtainMountainHeight(int x, int z, double** height) {
//    vec2 xz = vec2(x, z);
//    float h = 0, amp = 0.5, freq=128.f;
//    for(int i=0; i<4; i++) {
//        float h1 = pow(PerlinNoise(xz/freq), 2.f);
//        h1 = 1.f - abs(h1); //-> o to 1
//        h += h1*amp;
//        amp *= 0.5;
//        freq *= 0.5;
//    }
//    float ret = glm::clamp(pow(h*128.f, 1.5f), 0.f, 128.f);
//    return floor(ret);

    return glm::clamp((int) pow(abs(height[z][x]), 1.3f), 0, 128);
}

int genFractalMountainHeights(double **height, int levels, int size) {
    // Fractal heights Gen
    for (int i = 0; i < size + 1; ++i) {
        height[i] = new double[size + 1];
        for (int j = 0; j < size + 1; ++j) {
            height[i][j] = 0.0;
        }
    }
    for (int lev = 0; lev < levels; ++lev) {
        int step = size / pow(2, lev);
        for (int y = 0; y < size + 1; y += step) {
            int jumpov = 1 - (y / step) % 2;
            if (lev == 0) jumpov = 0;
            for (int x = step * jumpov; x < size + 1; x += step * (1 + jumpov)) {
                int pointer = 1 - (x / step) % 2 + 2 * jumpov;
                if (lev == 0) pointer = 3;
                int yref = step * (1 - pointer / 2);
                int xref = step * (1 - pointer % 2);
                double c1 = height[y - yref][x - xref];
                double c2 = height[y + yref][x + xref];
                double avg = (c1 + c2) / 2.0;
                double var = step * (glm::linearRand(0.0, 1.0) - 0.5);
                height[y][x] = (lev > 0) ? avg + var : 0;
            }
        }
    }
}

int obtainGrasslandHeight(int x, int z) {
    vec2 xz = vec2(x, z);
    float freq = 85.f;
    float h1 = PerlinNoise(xz/freq);
    h1 = 1.f - abs(h1);
    return floor(h1 * 22.f);
}


void Terrain::CreateTestScene()
{
    int xMin = 0, xMax = 256;
    int zMin = 0, zMax = 256;
    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = xMin; x < xMax; x += 16) {
        for(int z = zMin; z < zMax; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    //TODO: m2: CHANGE THIS
    m_generatedTerrain.insert(toKey(0, 0));

    int base_height = 128;
    int water_level = 148;
    int snow_level = 220;


    int levels = 9;
    int size = pow(2, (levels - 1));
    double **m_height = new double*[size + 1];
    genFractalMountainHeights(m_height, levels, size);


    // Create the basic terrain floor
    for(int x = xMin; x < xMax; ++x) {
        for(int z = zMin; z < zMax; ++z) {
            // Procedural biome - interp the heights - with very low freq
//            int y_m = obtainMountainHeight(x, z);
            int y_m = obtainMountainHeight(x, z, m_height);
            int y_g = obtainGrasslandHeight(x, z);
            float t = worleyNoise(vec2(x, z)*0.005f, 1.f);
            t = glm::smoothstep(0.35f, 0.75f, t);

            int interp_h = glm::clamp((int) glm::mix(y_g, y_m, t), 0, base_height-1);
            for (int k = 0; k<=interp_h; k++) {
                if (t>0.5) {
                    // Mountain biome
                    if (k+base_height >= snow_level && k == interp_h) {
                        setBlockAt(x, k+base_height, z, SNOW);
                    } else {
                        setBlockAt(x, k+base_height, z, STONE);
                    }
                } else {
                    // Grassland biome
                    if (k == interp_h) {
                        setBlockAt(x, k+base_height, z, GRASS);
                    }
                    else {
                        setBlockAt(x, k+base_height, z, DIRT);
                    }
                }
            }

            if (interp_h + base_height < water_level) {
                // Water level
                for (int kw=interp_h+base_height; kw < water_level; kw++) {
                    setBlockAt(x, kw, z, WATER);
                }
            }
        }
    }
}
