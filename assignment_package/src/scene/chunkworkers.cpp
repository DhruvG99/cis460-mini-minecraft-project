#include "chunkworkers.h"
#include "glm/gtc/random.hpp"
#include <iostream>
#include <QThreadPool>

using namespace glm;

vec2 random2(vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)))) * 43758.54f);
}

vec3 random3(vec3 p ) {
    float d1 = dot(p,vec3(127.1,311.7,234.0));
    float d2 = dot(p,vec3(269.5,183.3, 122.4));
    float d3 = dot(p,vec3(284.4,185.3, 199.3));
    vec3 s = sin(vec3(d1, d2, d3))* 43758.5f;
    return fract(s);
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

vec3 pow3d(vec3 t, float f) {
    float t1 = pow(t.x, f);
    float t2 = pow(t.y, f);
    float t3 = pow(t.z, f);
    return vec3(t1, t2, t3);
}

float surflet3D(vec3 P, vec3 gridPoint) {
    vec3 t2 = abs(P-gridPoint) * 1.f;
    vec3 t = vec3(1.f) - 6.f * pow3d(t2, 5.f) + 15.f * pow3d(t2, 4.f) - 10.f * pow3d(t2, 3.f);
    vec3 gradient = random3(gridPoint)* 2.f - vec3(1.f);
    vec3 diff = P - gridPoint;
    float height = dot(diff, gradient);
    return height * t.x * t.y * t.z;
}

float PerlinNoise3D(vec3 uvw) {
    float surfletsum = 0.f;
    for (int dx=0; dx <= 1 ; dx++) {
        for(int dy=0; dy <= 1; dy++) {
            for(int dz=0; dz <= 1; dz++) {
                surfletsum += surflet3D(uvw, floor(uvw) + vec3(dx, dy, dz));
            }
        }
    }
    return surfletsum;
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
    return glm::clamp((int) pow(abs(height[z][x]), 1.3f), 0, 128);
}

bool isGridEmpty(int x, int y, int z)
{
    vec3 xyz = vec3(x,y,z);
    float freq = 20.f, sum=0.f;
    for(int i=0; i<2; i++) {
        float noise = PerlinNoise3D(xyz/freq);
        sum+=noise;
        freq /= 2.f;
    }
    float threshold = 0.f;
    return (sum < threshold);
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


FBMWorker::FBMWorker(int x, int z, std::vector<Chunk*> chunksToFill,
                     std::unordered_set<Chunk*>* chunksFilled, QMutex* fillLock)
    : terrCoords(x,z),
      m_chunksToFill(chunksToFill),
      m_chunksFilled(chunksFilled),
      m_chunksFillLock(fillLock)
{

}

//iterate through all chunks that require BlockType Data for
//this terrain zone (4by4 chunks): obtained from m_chunksToFill
void FBMWorker::run()
{
    int base_height = 128;
    int water_level = 148;
    int snow_level = 220;
    int bed_level = 100;
    int lava_level = 10;
    int cave_opening_level = 155;

    int levels = 9;
    int size = pow(2, (levels - 1));
    double **m_height = new double*[size + 1];
    genFractalMountainHeights(m_height, levels, size);

    for(auto& chunk: this->m_chunksToFill)
    {
        //filling shared resource after loop

//        m_chunksFillLock->lock();
//        m_chunksFilled->insert(chunk);
//        m_chunksFillLock->unlock();

        int xChunk = chunk->m_xChunk;
        int zChunk = chunk->m_zChunk;

        // Create the basic terrain floor
        for(int x = 0; x < 16; ++x) {
            for(int z = 0; z < 16; ++z) {
                // set cave systems
                chunk->setBlockAt(x, bed_level, z, BEDROCK);
                for(int y=bed_level+1; y<base_height; y++) {
                    // Perlin Caves
                    if (!isGridEmpty(x, y, z)) {
                        chunk->setBlockAt(x, y, z, STONE);
                    } else if(y<bed_level+lava_level) {
                        chunk->setBlockAt(x, y, z, LAVA);
                    }
                }
                // Procedural biome - interp the heights - with very low freq
                //            int y_m = obtainMountainHeight(x, z);
                int y_m = obtainMountainHeight(abs(x+xChunk)%255, abs(z+zChunk)%255, m_height);
                int y_g = obtainGrasslandHeight(x+xChunk, z+zChunk);
                float t = worleyNoise(vec2(x+xChunk, z+zChunk)*0.005f, 1.f);
                t = glm::smoothstep(0.35f, 0.75f, t);

                int interp_h = glm::clamp((int) glm::mix(y_g, y_m, t), 0, base_height-1);

                if (interp_h + base_height < water_level) {
                    // Water level
                    for (int kw=interp_h+base_height; kw < water_level; kw++) {
                        chunk->setBlockAt(x, kw, z, WATER);
                    }
                }

                for (int k = 0; k<=interp_h; k++) {
                    if (t>0.5) {
                        // Mountain biome
                        if (k+base_height >= snow_level && k == interp_h) {
                            chunk->setBlockAt(x, k+base_height, z, SNOW);
                        } else {
                            chunk->setBlockAt(x, k+base_height, z, STONE);
                        }
                    } else {
                        // Grassland biome
                        if (isGridEmpty(x, k+base_height, z) && interp_h + base_height > water_level && interp_h + base_height < cave_opening_level) {
                            continue;
                        }
                        if (k == interp_h) {
                            chunk->setBlockAt(x, k+base_height, z, GRASS);
                        }
                        else {
                            chunk->setBlockAt(x, k+base_height, z, DIRT);
                        }
                    }
                }
                //            // Terrain for basic testing
                //            setBlockAt(x, bed_level, z, BEDROCK);
                //            setBlockAt(x, bed_level+1, z, LAVA);
                //            setBlockAt(x, bed_level+2, z, LAVA);
                //            setBlockAt(x, bed_level+3, z, LAVA);
                //            setBlockAt(x, bed_level+4, z, LAVA);
            }
        }
    }
    for (int i = 0; i <= size; i++) {
            delete[] m_height[i];
    }
    delete[] m_height;

    m_chunksFillLock->lock();
    for(auto& chunk: m_chunksToFill)
    {
        m_chunksFilled->insert(chunk);
    }
    m_chunksFillLock->unlock();


}

VBOWorker::VBOWorker(Chunk* c, std::vector<ChunkVBOData>* dat, QMutex* datLock, int t)
    : m_chunk(c),
      m_chunkVBOsCompleted(dat),
      m_chunkVBOsLock(datLock),
      time(t)
{

}

void VBOWorker::run()
{
    ChunkVBOData cvbo(m_chunk);
    m_chunk->createChunkVBOdata(cvbo, this->time);

    m_chunkVBOsLock->lock();
    m_chunkVBOsCompleted->push_back(cvbo);
    m_chunkVBOsLock->unlock();

}
