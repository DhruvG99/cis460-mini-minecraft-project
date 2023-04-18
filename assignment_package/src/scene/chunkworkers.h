#ifndef CHUNKWORKERS_H
#define CHUNKWORKERS_H

#include "glm_includes.h"
#include <QRunnable>
#include <QMutex>
#include <unordered_set>
#include "chunk.h"


class FBMWorker: public QRunnable
{
private:
    glm::ivec2 terrCoords;
    //chunks that have been instantiated but
    //need their BlockType data filled.
    std::vector<Chunk*> m_chunksToFill;
    //Chunks that have their BLockType data filled already
    std::unordered_set<Chunk*>* m_chunksFilled;
    QMutex* m_chunksFillLock;
public:
    FBMWorker(int, int, std::vector<Chunk*>, std::unordered_set<Chunk*>*, QMutex*);
    //fills blocktype data in chunksToFill
    void run() override;

};

class VBOWorker: public QRunnable
{
private:
    Chunk* m_chunk;
    std::vector<ChunkVBOData>* m_chunkVBOsCompleted;
    QMutex* m_chunkVBOsLock;
    int time;
public:
    VBOWorker(Chunk*, std::vector<ChunkVBOData>*, QMutex*, int);
    //calls the createVBO functions and everything
    void run() override;
};

#endif // CHUNKWORKERS_H
