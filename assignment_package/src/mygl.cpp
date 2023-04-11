#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_crosshair(this),
      m_progLambert(this), m_progFlat(this), m_progFlatCrosshair(this), m_progInstanced(this), m_texture(this),
      m_time(0), m_terrain(this), m_player(glm::vec3(52.f, 150.f, 42.f), m_terrain),
      m_framebuffer(this, width(), height(), devicePixelRatio()),
      m_progPostProcessCurrent(this),
      m_geomQuad(this), m_currFrameTime(QDateTime::currentMSecsSinceEpoch())
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    m_crosshair.setVals(50, width(), height());
    m_crosshair.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progFlatCrosshair.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    m_geomQuad.createVBOdata();
    m_progPostProcessCurrent.create(":/glsl/postproc.vert.glsl", ":/glsl/postproc.frag.glsl");
    m_framebuffer.create();

    // Load the texture image
    char* texturePath = ":/textures/minecraft_textures_all.png";
    m_texture.create(texturePath);
    m_texture.load(0);

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
//    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);


    m_player.rotateOnRightLocal(-60.f);
    m_player.moveForwardLocal(-50.f);

    m_terrain.CreateTestScene();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);

    m_progPostProcessCurrent.setDimensions(glm::ivec2(w * devicePixelRatio(), h*devicePixelRatio(   )));
    m_framebuffer.resize(w, h, 1);
    m_framebuffer.destroy();
    m_framebuffer.create();

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    long long prevTime = m_currFrameTime;
    m_currFrameTime = QDateTime::currentMSecsSinceEpoch();
    float dT = (m_currFrameTime - prevTime) * 0.001f; // in seconds
    m_player.tick(dT, m_inputs);

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    m_progLambert.setTime(m_time++);

    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setModelMatrix(glm::mat4());

    int texture_slot = 1;

   // Added
   glm::vec3 playerPos = m_player.mcr_camera.mcr_position;
   auto currBtype = m_terrain.getBlockAt(playerPos.x, playerPos.y, playerPos.z);
   std::cout << int(currBtype) << std::endl;
   if (currBtype == WATER || currBtype == LAVA) {
       m_framebuffer.bindFrameBuffer();
       glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
       glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       m_framebuffer.bindToTextureSlot(texture_slot);
    }

    m_texture.bind(0);
    renderTerrain();

    // Added
    if (currBtype == WATER || currBtype == LAVA) {
        glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
        glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_framebuffer.bindToTextureSlot(texture_slot);

        // if check on the draw post
        m_progPostProcessCurrent.setBlckType(int(currBtype));
        m_progPostProcessCurrent.drawPost(m_geomQuad, texture_slot);
        m_progPostProcessCurrent.setTime(m_time);
    }

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);

    m_progFlatCrosshair.setModelMatrix(glm::mat4());
    m_progFlatCrosshair.setViewProjMatrix(glm::mat4());
    m_progFlatCrosshair.draw(m_crosshair);
    glEnable(GL_DEPTH_TEST);


}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    glm::vec3 currPos = m_player.mcr_position;
    int currX = static_cast<int>(glm::floor(currPos.x / 16.f));
    int currZ = static_cast<int>(glm::floor(currPos.z / 16.f));
    //for 3 by 3 chunks around player

    m_terrain.draw(16*currX-32, 16*currX+32, 16*currZ-32, 16*currZ+32, &m_progLambert, m_time);
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    if(e->modifiers() & Qt::ShiftModifier){
        m_inputs.shiftPressed = true;
    }
    else{
        m_inputs.shiftPressed = false;
    }

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    } else if (e->key() == Qt::Key_F) {
        m_player.toggleFlyMode();
    }
}


void MyGL::keyReleaseEvent(QKeyEvent *e){
    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    m_inputs.prevMouseX = this->width() / 2;
    m_inputs.prevMouseY = this->height() / 2;
    m_inputs.mouseX = e->x();
    m_inputs.mouseY = e->y();
    moveMouseToCenter();

}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::RightButton){
        m_player.placeBlock(m_terrain);
    }
    else if(e->button() == Qt::LeftButton){
        m_player.breakBlock(m_terrain);
    }
}
