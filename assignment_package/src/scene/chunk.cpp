#include "chunk.h"
#include <iostream>

Chunk::Chunk(OpenGLContext* context, int x, int z) :
    Drawable(context), m_blocks(),
    m_neighbors{{XPOS, nullptr},{XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    m_xChunk(x), m_zChunk(z)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

Chunk::~Chunk()
{
    this->destroyVBOdata();
}

//Using x,z - the chunk coordinate - to transform all blocks appropriately
void Chunk::createChunkVBOdata(ChunkVBOData &c)
{
    m_idxCount = 0;
    m_idxInter.clear();
    m_vboInter.clear();

    c.m_idxTrans.clear();
    c.m_vboTrans.clear();
    c.m_idxOpaque.clear();
    c.m_vboOpaque.clear();

    //bools - vbo for chunk gen or not- > render
    for(int i = 0; i < 16; ++i) {
        for(int j = 0; j < 256; ++j) {
            for(int k = 0; k < 16; ++k) {
                BlockType currBlock = this->getBlockAt(i, j, k);
                //if not empty, paint faces (while checking for empty neighbors)
                if(currBlock != EMPTY)
                {
                    for(const BlockFace &f: adjacentFaces)
                    {
                        BlockType adjBlock = EMPTY;
                        glm::vec3 testBorder = glm::vec3(i,j,k) + f.dirVec;
                        /*
                         * testing if the face is on a bordering chunk
                         * and then assigning to adjacent block
                        */
                        if(testBorder.x >=16.f || testBorder.y >=256.f || testBorder.z >=16.f ||
                                testBorder.x < 0.0f || testBorder.y < 0.0f || testBorder.z < 0.f)
                        {
                            Chunk* adjChunk = this->m_neighbors[f.dir];
                            //if a neighbo(u)ring chunk exists
                            if(adjChunk != nullptr) {
                                //https://stackoverflow.com/questions/7594508/modulo-operator-with-negative-values
                                int dx = (16 + i + (int)f.dirVec.x) % 16;
                                int dy = (256 + j + (int)f.dirVec.y) % 256;
                                int dz = (16 + k + (int)f.dirVec.z) % 16;
                                adjBlock = adjChunk->getBlockAt(dx,dy,dz);
                            }
                        }
                        //face is in the same chunk
                        else
                            adjBlock = this->getBlockAt(i+(int)f.dirVec.x, j+(int)f.dirVec.y, k+(int)f.dirVec.z);

                        //transparent block
                        if(currBlock == WATER)
                        {
                            //possibly check if the adjBlock is water? (transparency)
                            if(adjBlock==EMPTY)
                            {
                                glm::vec4 vertCol = colorFromBlock.at(DEBUG);
                                if(colorFromBlock.count(currBlock) != 0)
                                    vertCol = colorFromBlock.at(currBlock);

                                //pos vecs for this block - last elem 0.0f because it adds to vert
                                glm::vec4 blockPos = glm::vec4(i+this->m_xChunk, j, k+this->m_zChunk, 0.0f);
                                for(const VertexData &v: f.verts)
                                {
                                    glm::vec4 vertPos = v.pos + blockPos;
                                    c.m_vboTrans.push_back(vertPos);
                                    c.m_vboTrans.push_back(vertCol);
                                    c.m_vboTrans.push_back(glm::vec4(f.dirVec,0.f));
                                }
                                c.m_idxTrans.push_back(0 + m_idxCount);
                                c.m_idxTrans.push_back(1 + m_idxCount);
                                c.m_idxTrans.push_back(2 + m_idxCount);
                                c.m_idxTrans.push_back(0 + m_idxCount);
                                c.m_idxTrans.push_back(2 + m_idxCount);
                                c.m_idxTrans.push_back(3 + m_idxCount);
                                m_idxCount += 4;
                            }
                        }
                        else
                        {
                            if(adjBlock==EMPTY || adjBlock==WATER)
                            {
                                glm::vec4 vertCol = colorFromBlock.at(DEBUG);
                                if(colorFromBlock.count(currBlock) != 0)
                                    vertCol = colorFromBlock.at(currBlock);

                                //pos vecs for this block - last elem 0.0f because it adds to vert
                                glm::vec4 blockPos = glm::vec4(i+this->m_xChunk, j, k+this->m_zChunk, 0.0f);
                                for(const VertexData &v: f.verts)
                                {
                                    glm::vec4 vertPos = v.pos + blockPos;
                                    c.m_vboOpaque.push_back(vertPos);
                                    c.m_vboOpaque.push_back(vertCol);
                                    c.m_vboOpaque.push_back(glm::vec4(f.dirVec,0.f));
                                }
                                c.m_idxOpaque.push_back(0 + m_idxCount);
                                c.m_idxOpaque.push_back(1 + m_idxCount);
                                c.m_idxOpaque.push_back(2 + m_idxCount);
                                c.m_idxOpaque.push_back(0 + m_idxCount);
                                c.m_idxOpaque.push_back(2 + m_idxCount);
                                c.m_idxOpaque.push_back(3 + m_idxCount);
                                m_idxCount += 4;
                            }
                        }
                    }
                }
            }
        }
    }
    m_idxInter.insert(m_idxInter.end(), c.m_idxOpaque.begin(), c.m_idxOpaque.end());
    m_idxInter.insert(m_idxInter.end(), c.m_idxTrans.begin(), c.m_idxTrans.end());
    m_vboInter.insert(m_vboInter.end(), c.m_vboOpaque.begin(), c.m_vboOpaque.end());
    m_vboInter.insert(m_vboInter.end(), c.m_vboTrans.begin(), c.m_vboTrans.end());
    this->m_count = m_idxInter.size();
}

void Chunk::createVBOdata()
{
    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idxInter.size() * sizeof(GLuint), m_idxInter.data(), GL_STATIC_DRAW);

    generateVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboInter.size() * sizeof(glm::vec4), m_vboInter.data(), GL_STATIC_DRAW);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}
