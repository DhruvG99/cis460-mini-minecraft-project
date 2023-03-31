#include "terrain.h"
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
//TODO: draw chunk border
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram)
{
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk!=nullptr)
            {
                chunk->createChunkVBOdata(x, z);
                chunk->createVBOdata();
                shaderProgram->drawInter(*chunk);
            }
        }
    }
}
#endif

#if 0
//USE THE BELOW TO REDO ABOVE. call draw at end of every chunk for loop
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    m_geomCube.clearOffsetBuf();
    m_geomCube.clearColorBuf();

    std::vector<glm::vec3> offsets, colors;

    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            for(int i = 0; i < 16; ++i) {
                for(int j = 0; j < 256; ++j) {
                    for(int k = 0; k < 16; ++k) {
                        BlockType t = chunk->getBlockAt(i, j, k);

                        if(t != EMPTY) {
                            offsets.push_back(glm::vec3(i+x, j, k+z));
                            switch(t) {
                            case GRASS:
                                colors.push_back(glm::vec3(95.f, 159.f, 53.f) / 255.f);
                                break;
                            case DIRT:
                                colors.push_back(glm::vec3(121.f, 85.f, 58.f) / 255.f);
                                break;
                            case STONE:
                                colors.push_back(glm::vec3(0.5f));
                                break;
                            case WATER:
                                colors.push_back(glm::vec3(0.f, 0.f, 0.75f));
                                break;
                            default:
                                // Other block types are not yet handled, so we default to debug purple
                                colors.push_back(glm::vec3(1.f, 0.f, 1.f));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    m_geomCube.createInstancedVBOdata(offsets, colors);
    shaderProgram->drawInstanced(m_geomCube);
}
#endif

vec2 random2(vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)))) * 43758.54f);
}
float surflet(vec2 P, vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial (quintic smootherstep function)
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1.0 - 6.0 * pow(distX, 5.0) + 15.0 * pow(distX, 4.0) - 10.0 * pow(distX, 3.0);
    float tY = 1.0 - 6.0 * pow(distY, 5.0) + 15.0 * pow(distY, 4.0) - 10.0 * pow(distY, 3.0);

    // Get the random vector for the grid point
    vec2 gradient = random2(gridPoint);
    // Get the vector from the grid point to P
    vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

float PerlinNoise(vec2 uv) {
    // Tile the space
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

int obtainMountainHeight(int x, int z) {
    vec2 xz = vec2(x, z);
    float h = 0, amp = 0.5, freq=128.f;
    for(int i=0; i<4; i++) {
        float h1 = PerlinNoise(xz/freq);
        h1 = 1.f - abs(h1); //-> o to 1
        h += h1*amp;
        amp *= 0.5;
        freq *= 0.5;
    }
    return floor(h * 128.f);
}

int obtainGrasslandHeight(int x, int z) {
    vec2 xz = vec2(x, z);
    float freq = 70.f;
    float h = worleyNoise(xz/freq, 0.5);
    return floor(h * 10.f);

//    float h1 = PerlinNoise(xz/freq);
//    h1 = 1.f - abs(h1);
//    return floor(h1 * 20.f);
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
    int water_level = 150;
    int snow_level = 200;
    // Create the basic terrain floor
        for(int x = xMin; x < xMax; ++x) {
            for(int z = zMin; z < zMax; ++z) {
                // Procedural biome - interp the heights - with very low freq
                int y_m = obtainMountainHeight(x, z);
                int y_g = obtainGrasslandHeight(x, z);
                float t = worleyNoise(vec2(x, z)*0.005f, 1.f);
    //            float t = glm::smoothstep(0.35f, 0.5f, (float)w);

                int interp_h = (int) glm::mix(y_g, y_m, t);
    //            cout << "height m, g, t, interp height:"<<y_m+128 << " "<<y_g+128 <<" "<< t<< " "<<interp_h + 128 << endl;
                for (int k = 0; k<=interp_h; k++) {
                    if (t>0.45) {
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
                    for (int kw=interp_h+base_height; kw<water_level; kw++) {
                        setBlockAt(x, kw, z, WATER);
                    }
                }
//                for(int ks=0; ks<128; ks++) {
//                    // Stone bed for the landscape below 128
//                    setBlockAt(x, ks, z, STONE);
//                }
            }
        }

#if 0
    // Create the basic terrain floor
        for(int x = 0; x < 64; ++x) {
            for(int z = 0; z < 64; ++z) {
                if((x + z) % 2 == 0) {
                    setBlockAt(x, 128, z, STONE);
                }
                else {
                    setBlockAt(x, 128, z, DIRT);
                }
            }
        }
        // Add "walls" for collision testing
        for(int x = 0; x < 64; ++x) {
            setBlockAt(x, 129, 0, GRASS);
            setBlockAt(x, 130, 0, GRASS);
            setBlockAt(x, 129, 63, GRASS);
            setBlockAt(0, 130, x, GRASS);
        }
        // Add a central column
        for(int y = 129; y < 140; ++y) {
            setBlockAt(32, y, 32, GRASS);
        }
#endif
}
