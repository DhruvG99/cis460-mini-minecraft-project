#include "terrain.h"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <QThreadPool>
#include "chunkworkers.h"

#define TERRAIN_ZONE_RADIUS 3

using namespace std::chrono;
using namespace glm;

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(),
      m_chunksThatHaveBlockData(), m_blockDataLock(),
      m_chunksThatHaveVBOData(), m_vboDataLock(),
      mp_context(context)
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
    uPtr<Chunk> chunk = mkU<Chunk>(this->mp_context, x, z);
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

//TODO: m3: draw chunk border?
void Terrain::draw(int currX, int currZ, ShaderProgram *shaderProgram)
{
    //for all terrains being rendered
    // so 3 terrains in each direction + self
    // so 7*7 = 49 terrains
    // 49*4*4 = 784 Chunks
    // 784*16*16 = 200,704 blocks in entire render
    int rad = TERRAIN_ZONE_RADIUS;
    for(int x = currX - 64*rad; x < currX + 64*(rad+1); x += 16) {
        for(int z = currZ - 64*rad; z < currZ + 64*(rad+1); z += 16) {
            const uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk!=nullptr) {
                shaderProgram->drawInter(*chunk);
            }

        }
    }
}

bool Terrain::terrainZoneExists(int64_t id)
{
    if(this->m_generatedTerrain.count(id)==0)
        return false;
    else
        return true;
}

std::unordered_set<int64_t> Terrain::findTerrainZoneArea(glm::ivec2 terrPos, int rad)
{
    std::unordered_set<int64_t> result;
    int radZone = static_cast<int>(rad)*64;
    //returns set of terrain zones inside the radius
    for(int x = -radZone; x <= radZone; x+=64) {
        for(int z = -radZone; z <= radZone; z+=64)
            result.insert(toKey(x + terrPos.x, z + terrPos.y));
    }
    return result;
}


//allocating VBO data for chunk
void Terrain::spawnVBOWorker(Chunk* chunk, int time)
{
    /*
    //spawn vbo worker
    */
    VBOWorker* worker = new VBOWorker(chunk,
                                  &m_chunksThatHaveVBOData,
                                  &m_vboDataLock,
                                  time);
    QThreadPool::globalInstance()->start(worker);
}

//creating chunks from scratch
void Terrain::spawnFBMWorker(int64_t terrZoneId)
{
    this->m_generatedTerrain.insert(terrZoneId);
    std::vector<Chunk*> chunksThatNeedBlockType;
    glm::ivec2 coords = toCoords(terrZoneId);
    //one terrain zone is 64 by 64, origin at 'coords'
    //VERIFIED: For 7by7 terrain zones, this instantiates
    //784 chunks (7*7*4*4)
    for(int x = coords.x; x < coords.x + 64; x+=16) {
        for(int z = coords.y; z < coords.y + 64; z+=16) {
            Chunk* chunk = instantiateChunkAt(x,z);
            //TODO: drawing logic
//            chunk->m_countTrans = 0;
//            chunk->m_countOpaque = 0;
            chunk->m_count = 0;
            chunksThatNeedBlockType.push_back(chunk);
        }
    }
    //FBM worker calls
    FBMWorker* worker = new FBMWorker(coords.x, coords.y,
                                      chunksThatNeedBlockType,
                                      &m_chunksThatHaveBlockData,
                                      &m_blockDataLock);
    QThreadPool::globalInstance()->start(worker);
}


//loading initial terrain with respect to
//initial position @ (x=52,z=42) => terrain origin @ (0,0)
//terrain: 7*7 terrain zones
void Terrain::loadInitialTerrain()
{
    glm::ivec2 currZonePos(0,0);
    std::unordered_set<int64_t> currZoneArea = findTerrainZoneArea(currZonePos, TERRAIN_ZONE_RADIUS);
    std::cout << "Zones: " << currZoneArea.size() << std::endl;

    for(auto id: currZoneArea)
        spawnFBMWorker(id);
}

/*
 * spawn FBM worker for border IDs,
 * storing in a different chunksTHBlockData vector
 * Create VBOs for all, area and border
 * But don't pass border to GPU
 * During try expansion -
 * If expansion needed
 *  destroy outside range
 *
 */

void Terrain::tryExpansion(glm::vec3 prevPos, glm::vec3 currPos, int time)
{
    if(prevPos==currPos)
        return;

    //check if moved to a new terrain zone since last tick
    glm::ivec2 prevZonePos(64*static_cast<int>(glm::floor(prevPos.x / 64.f)),
                        64*static_cast<int>(glm::floor(prevPos.z / 64.f)));
    glm::ivec2 currZonePos(64*static_cast<int>(glm::floor(currPos.x / 64.f)),
                        64*static_cast<int>(glm::floor(currPos.z / 64.f)));

    //Terrain Zone Radius SET TO 3 FOR NOW
    //USING RADIUS+1 to account for border VBO (not rendered)
    std::unordered_set<int64_t> currZoneArea = findTerrainZoneArea(currZonePos, TERRAIN_ZONE_RADIUS);
    std::unordered_set<int64_t> prevZoneArea = findTerrainZoneArea(prevZonePos, TERRAIN_ZONE_RADIUS);

    //destroying terrain zones/chunks that are not in radius anymore
    for(auto id: prevZoneArea) {
        //not in new terrain zones
        if(currZoneArea.count(id)==0)
        {
            glm::ivec2 coord = toCoords(id);
            for(int x = coord.x; x < coord.x + 64; x++) {
                for(int z = coord.y; z < coord.y + 64; z++) {
                    auto &chunk = getChunkAt(x,z);
                    chunk->destroyVBOdata();
                }
            }
        }
    }

    //assigning VBO Data to terrain zones that
    //have been visited before but their VBO data was destroyed
    //- By default, We can assume the VBO data was destroyed
    // because as coded above, the program will destroy any VBO data outside the zone
    for(auto id: currZoneArea) {
        if(terrainZoneExists(id)) {
            if(prevZoneArea.count(id)==0) {
                glm::ivec2 coord = toCoords(id);
                for(int x = coord.x; x < coord.x + 64; x+=16) {
                    for(int z = coord.y; z < coord.y + 64; z+=16) {
                        auto &chunk = getChunkAt(x,z);
                        chunk->m_count = 0;
                        //this should reallocate VBOs
                        spawnVBOWorker(chunk.get(), time);
                    }
                }
            }
        }
        //creating blocktype Data for terrain zones/chunks that
        //have NEVER been visited before - from scratch
        else {
            spawnFBMWorker(id);
        }
    }
}


void Terrain::checkThreadResults(int time)
{
    //After all FBM workers are done, we now create VBO data
    //for the newly instantiated chunks which have BlockType data
    //Adds to m_chunksThatHaveVBOData bts
    //createChunkVBOdata called here and
    // chunksThatHaveVBOData is populated
    this->m_blockDataLock.lock();
    for(auto chunk: m_chunksThatHaveBlockData)
        spawnVBOWorker(chunk, time);
    m_chunksThatHaveBlockData.clear();
    this->m_blockDataLock.unlock();

    //Now, all chunks that have VBO data and can be sent to GPU
    this->m_vboDataLock.lock();
    for(ChunkVBOData& c: m_chunksThatHaveVBOData)
    {
        c.m_chunk->createVBOdata();
    }
    m_chunksThatHaveVBOData.clear();
    this->m_vboDataLock.unlock();
}
