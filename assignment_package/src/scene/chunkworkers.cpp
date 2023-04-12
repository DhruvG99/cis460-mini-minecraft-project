#include "chunkworkers.h"
#include "glm/gtc/random.hpp"
#include <iostream>
using namespace glm;

vec2 random2(vec2 p) {
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
    std::cout << "Chunks To fill: "<< m_chunksToFill.size() << std::endl;
    for(auto chunk: this->m_chunksToFill)
    {
        m_chunksFillLock->lock();
        m_chunksFilled->insert(chunk);
        m_chunksFillLock->unlock();

        int xChunk = chunk->m_xChunk;
        int zChunk = chunk->m_zChunk;

        int base_height = 128;
        int water_level = 148;
        int snow_level = 220;


        int levels = 9;
        int size = pow(2, (levels - 1));
        double **m_height = new double*[size + 1];
        genFractalMountainHeights(m_height, levels, size);


        // Create the basic terrain floor
        for(int x = 0; x < 16; ++x) {
            for(int z = 0; z < 16; ++z) {
                // Procedural biome - interp the heights - with very low freq
    //            int y_m = obtainMountainHeight(x, z);
                int y_m = obtainMountainHeight(x+xChunk, z+zChunk, m_height);
                int y_g = obtainGrasslandHeight(x+xChunk, z+zChunk);
                float t = worleyNoise(vec2(x+xChunk, z+zChunk)*0.005f, 1.f);
                t = glm::smoothstep(0.35f, 0.75f, t);

                int interp_h = glm::clamp((int) glm::mix(y_g, y_m, t), 0, base_height-1);
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
                        if (k == interp_h) {
                            chunk->setBlockAt(x, k+base_height, z, GRASS);
                        }
                        else {
                            chunk->setBlockAt(x, k+base_height, z, DIRT);
                        }
                    }
                }

                if (interp_h + base_height < water_level) {
                    // Water level
                    for (int kw=interp_h+base_height; kw < water_level; kw++) {
                        chunk->setBlockAt(x, kw, z, WATER);
                    }
                }
            }
        }
    }

}

VBOWorker::VBOWorker(Chunk* c, std::vector<ChunkVBOData>* dat, QMutex* datLock)
    : m_chunk(c),
      m_chunkVBOsCompleted(dat),
      m_chunkVBOsLock(datLock)
{

}

void VBOWorker::run()
{
    ChunkVBOData cvbo(m_chunk);
    m_chunk->createChunkVBOdata(cvbo);

    m_chunkVBOsLock->lock();
    m_chunkVBOsCompleted->push_back(cvbo);
    m_chunkVBOsLock->unlock();
}
