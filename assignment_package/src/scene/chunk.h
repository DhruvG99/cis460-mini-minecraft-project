#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>

#define BLK_UVX * 0.0625f
#define BLK_UVY * 0.0625f

//= 1/32; Will be helpful later on to obtain texture from the texture pack using local coords
//#define BLK_UV 0.03125f

//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, DEBUG, LAVA, ICE, BEDROCK
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
        XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

struct VertexData {
    glm::vec4 pos;
    VertexData(glm::vec4 p)
        : pos(p)
    {}
};

struct BlockFace {
    Direction dir;
    glm::vec3 dirVec;
    std::array<VertexData, 4> verts;

    BlockFace(Direction de, glm::vec3 dv, const VertexData &a, const VertexData &b, const VertexData &c, const VertexData &d)
        : dir(de), dirVec(dv), verts{a,b,c,d}
    {}
};

const static std::array<BlockFace, 6> adjacentFaces {
    BlockFace(XPOS, glm::vec3(1,0,0), VertexData(glm::vec4(1,0,1,1)),
                                      VertexData(glm::vec4(1,0,0,1)),
                                      VertexData(glm::vec4(1,1,0,1)),
                                      VertexData(glm::vec4(1,1,1,1))),

    BlockFace(XNEG, glm::vec3(-1,0,0), VertexData(glm::vec4(0,0,0,1)),
                                      VertexData(glm::vec4(0,0,1,1)),
                                      VertexData(glm::vec4(0,1,1,1)),
                                      VertexData(glm::vec4(0,1,0,1))),

    BlockFace(YPOS, glm::vec3(0,1,0), VertexData(glm::vec4(0,1,1,1)),
                                      VertexData(glm::vec4(1,1,1,1)),
                                      VertexData(glm::vec4(1,1,0,1)),
                                      VertexData(glm::vec4(0,1,0,1))),

    BlockFace(YNEG, glm::vec3(0,-1,0), VertexData(glm::vec4(0,0,0,1)),
                                      VertexData(glm::vec4(1,0,0,1)),
                                      VertexData(glm::vec4(1,0,1,1)),
                                      VertexData(glm::vec4(0,0,1,1))),

    BlockFace(ZPOS, glm::vec3(0,0,1), VertexData(glm::vec4(0,1,1,1)),
                                      VertexData(glm::vec4(0,0,1,1)),
                                      VertexData(glm::vec4(1,0,1,1)),
                                      VertexData(glm::vec4(1,1,1,1))),

    BlockFace(ZNEG, glm::vec3(0,0,-1), VertexData(glm::vec4(1,1,0,1)),
                                      VertexData(glm::vec4(1,0,0,1)),
                                      VertexData(glm::vec4(0,0,0,1)),
                                      VertexData(glm::vec4(0,1,0,1)))
};

// TODO: shubh: add ice color here
//color map to easily retrieve colors of different blocks
const static std::unordered_map<BlockType, glm::vec4, EnumHash> colorFromBlock = {
  {GRASS, glm::vec4(glm::vec3(95.f, 159.f, 53.f)/255.f, 1.0f)},
  {DIRT, glm::vec4(glm::vec3(121.f, 85.f, 58.f)/255.f, 1.0f)},
  {STONE, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)},
  {WATER, glm::vec4(0.f, 0.f, 0.75f, 1.0f)},
  {SNOW, glm::vec4(1.0f)},
  {LAVA, glm::vec4(1,0,0,1)},
  {BEDROCK, glm::vec4(1,1,0,1)},
  {DEBUG, glm::vec4(1.f, 0.f, 1.f, 1.0f)}
};

// map for textures
const static std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> BlockFaceUVs = {
    {GRASS, std::unordered_map<Direction, glm::vec2, EnumHash> { {XPOS, glm::vec2(3.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {XNEG, glm::vec2(3.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {YPOS, glm::vec2(8.f BLK_UVX , 13.f BLK_UVY)},
                                                                 {YNEG, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {ZPOS, glm::vec2(3.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {ZNEG, glm::vec2(3.f BLK_UVX , 15.f BLK_UVY)}
                                                               },
     },

     {DIRT, std::unordered_map<Direction, glm::vec2, EnumHash> { {XPOS, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                {XNEG, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                {YPOS, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                {YNEG, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                {ZPOS, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)},
                                                                {ZNEG, glm::vec2(2.f BLK_UVX , 15.f BLK_UVY)}
                                                              },
     },

     {STONE, std::unordered_map<Direction, glm::vec2, EnumHash> { {XPOS, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {XNEG, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {YPOS, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {YNEG, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {ZPOS, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)},
                                                                 {ZNEG, glm::vec2(1.f BLK_UVX , 15.f BLK_UVY)}
                                                               },
     },

     {WATER, std::unordered_map<Direction, glm::vec2, EnumHash> { {XPOS, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)},
                                                                 {XNEG, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)},
                                                                 {YPOS, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)},
                                                                 {YNEG, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)},
                                                                 {ZPOS, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)},
                                                                 {ZNEG, glm::vec2(13.f BLK_UVX , 3.f BLK_UVY)}
                                                               },
     },

    {SNOW, std::unordered_map<Direction, glm::vec2, EnumHash> {   {XPOS, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)},
                                                                 {XNEG, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)},
                                                                 {YPOS, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)},
                                                                 {YNEG, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)},
                                                                 {ZPOS, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)},
                                                                 {ZNEG, glm::vec2(3.f BLK_UVX , 11.f BLK_UVY)}
                                                               },
     },

     {DEBUG, std::unordered_map<Direction, glm::vec2, EnumHash> { {XPOS, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)},
                                                                 {XNEG, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)},
                                                                 {YPOS, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)},
                                                                 {YNEG, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)},
                                                                 {ZPOS, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)},
                                                                 {ZNEG, glm::vec2(15.f BLK_UVX , 14.f BLK_UVY)}
                                                               }
     }
};


// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    int idxCount = 0;
    std::vector<GLuint> idx;
    std::vector<glm::vec4> vboInter;

public:
    Chunk(OpenGLContext*);
    ~Chunk();
    void createChunkVBOdata(int, int, int time, bool getTransparent=false);
    void createVBOdata() override;
    //drawMode is triangles by default

    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
};
