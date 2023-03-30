#pragma once
#include <glm_includes.h>
#include "openglcontext.h"
#include "drawable.h"

//class MyGL: public OpenGLContext{};

class Crosshair : public Drawable{
    int m_crosshair_length;
    int m_screen_width;
    int m_screen_height;
public:
    virtual ~Crosshair() override;
    Crosshair(OpenGLContext* context) : Drawable(context) {}
    void createVBOdata() override;
    GLenum drawMode() override;
    void setVals(int, int, int);
    void getXY(int half_length, int width, int height, float &out_x, float &out_y);
};
