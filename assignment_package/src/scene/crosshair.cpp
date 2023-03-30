#include "crosshair.h"

Crosshair::~Crosshair()
{}

GLenum Crosshair::drawMode(){
    return GL_LINES;
}

void Crosshair::setVals(int crosshair_length, int width, int height){
    m_crosshair_length = crosshair_length;
    m_screen_width = width;
    m_screen_height = height;
}

void Crosshair::createVBOdata()
{
    float fracX, fracY;
    getXY(m_crosshair_length/2, m_screen_width, m_screen_height, fracX, fracY);
    GLuint idx[4] = {0, 1, 2, 3};
    // the sizes were calculated using algebra.
    glm::vec4 pos[4] = {glm::vec4(0, -fracY, 0, 1), glm::vec4(0, fracY, 0, 1),
                        glm::vec4(-fracX, 0, 0, 1), glm::vec4(fracX, 0, 0, 1)};
    glm::vec4 col[4] = {glm::vec4(1,1,1,1), glm::vec4(1,1,1,1),
                        glm::vec4(1,1,1,1), glm::vec4(1,1,1,1)};

    m_count = 4;

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), idx, GL_STATIC_DRAW);
    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), pos, GL_STATIC_DRAW);
    generateCol();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufCol);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), col, GL_STATIC_DRAW);
}

void Crosshair::getXY(int half_length, int width, int height, float &out_x, float &out_y){
    float pos_x = width/2 + half_length;
    out_x = (pos_x / width * 2) - 1;

    float pos_y = height/2 + half_length;
    out_y = 1 - (pos_y / height * 2);
}
