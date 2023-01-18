// Final Project: Marching Cubes
// CSCI 4239
// Jess Hamlin

// Includes
#include "CSCIx239.h"
#include <time.h> 

#define CFD 20
#define CD 1
#define CPFD 20         // Cubes in each chunk
#define CPD 1           // Size of each cube
#define CUBESAROUND 6   // Cubes around the camera in each direction

// Globals
float asp = 1;          // Aspect ratio
int fov = 57;           // Field of view (for perspective)
float dimA = CFD * CD;  // Size of world
float dimB = CPFD * CPD;// Size of world
int th = 180, ph = 20;  // View angles
float zh = 0;           // Changes over time
int move = 1;           // Moving light
int mode = 0;           // Mode of display
int rumble = 0;         // Bool for if to play rumble animation
// int pulse = 0;          // Bool for direction of pulse animation
// float pulseTime = 0;    // Timer for pulse animation
int score = 0;          // Keep track of score
int tab = 0;            // If tab is pressed
int tex = 0;            // Help texture

float isolevel = 0.5;
float currentCube = 0;
int cubeStep = 1;

int shader;
int bloomShader;
int bloomShader2;

// Buffer stuff
unsigned int depthbuf = 0;  // Depth buffer
unsigned int img[4] = {0};  // Image textures
unsigned int framebuf[3];   // Frame buffers
int NP=1;                   // Number of passes

// Movement
int w, a, s, d = 0;
int u, l, r, dwn = 0;

// Structs

// Vectors
typedef struct {
    float x;
    float y;
    float z;
} vec3;
typedef struct {
    float x;
    float y;
    float z;
    float w;
} vec4;

// Cube cube cube cube cube™
typedef struct {
    vec3 verts[8];
    float weights[8];
} cube;

// Cube array
cube cf[CFD * CFD * CFD];

// Camera values
vec3 camPos;
vec3 camRot;

// Goal
vec3 coinPos;

// Contents (In order of appearance)
// int main(int argc, char* argv[])
void reshape(GLFWwindow* window, int width, int height);
void key(GLFWwindow* window, int key, int scancode, int action, int mods);
void placeCoin();
void drawCube(cube c);
void moveCube(cube* c, float x, float y, float z);
void makeTri(cube c, int e1, int e2, int e3);
void getMesh(cube c);
void makePoints(float ox, float oy, float oz);
void makeWorld();
void display(GLFWwindow* window);

// Main
int main(int argc, char* argv[])
{
    // Initialize GLFW
    // Arguments are: Window title, vsync, width, height, reshape, and key
    GLFWwindow* window = InitWindow("Final Project: Jess Hamlin", 1, 800, 800, &reshape, &key);

    // Get shaders
    shader = CreateShaderProgGeom("basic.vert", "cubeMarcher3000.geom", "basic.frag");
    bloomShader = CreateShaderProgGeom("basic.vert", "cubeMarcher3000.geom", "bloom.frag");
    bloomShader2 = CreateShaderProg(NULL,"bloom2.frag");

    // FOR MODE 1
    // Make cube array
    vec3 cArr[8] = {
        {0, 0, CD}, {0, CD, CD}, {CD, CD, CD}, {CD, 0, CD},
        {0, 0, 0}, {0, CD, 0}, {CD, CD, 0}, {CD, 0, 0}
    }; // 8 verts of a cube
    int len = 0;
    int wid = 0;
    int dep = -CD;
    for (int i = 0; i < CFD * CFD * CFD; i++) {
        cube c;
        for (int i = 0; i < 8; i++) {
            c.weights[i] = 0;
        }
        memcpy(c.verts, cArr, sizeof(cArr));
        len += CD;
        if (i % CFD == 0) {
            wid += CD;
            len = 0;
        }
        if (i % (CFD * CFD) == 0) {
            dep += CD;
            wid = 0;
        }
        moveCube(&c, len, wid, dep);

        cf[i] = c;
    }
    // Set pattern as distance from middle
    for (int i = 0; i < CFD * CFD * CFD; i++) {
        for (int j = 0; j < 8; j++) {
            if (sqrt(pow(cf[i].verts[j].x - (CFD / 2), 2) + pow(cf[i].verts[j].y - (CFD / 2), 2) + pow(cf[i].verts[j].z - (CFD / 2), 2)) < CFD / 2) {
                cf[i].weights[j] = 1;
            }
        }
    }

    // FOR MODE 0
    // Initialize camera
    camPos.x = 0;
    camPos.y = 0;
    camPos.z = 0;
    camRot.x = 0;
    camRot.y = 0;
    camRot.z = 0;

    // Get texture
    tex = LoadTexBMP("cover.bmp");

    // Seed randomness
    time_t t;
    srand((unsigned) time(&t));
    // Place objective
    placeCoin();

    // Event loop
    ErrCheck("init");
    while(!glfwWindowShouldClose(window))
    {
        // Display
        display(window);
        // Process any events
        glfwPollEvents();
    }
    //  Shut down GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// Window resized callback
void reshape(GLFWwindow* window, int width, int height)
{
    //  Get framebuffer dimensions (makes Apple work right)
    glfwGetFramebufferSize(window,&width,&height);
    //  Ratio of the width to the height of the window
    asp = (height>0) ? (double)width/height : 1;
    //  Set the viewport to the entire window
    glViewport(0,0, width,height);
    //
    //  Allocate a frame buffer
    //  Typically the same size as the screen (W,H) but can be larger or smaller
    //
    //  Delete old frame buffer, depth buffer and texture
    if (depthbuf)
    {
        glDeleteRenderbuffers(1,&depthbuf);
        glDeleteTextures(4,img);
        glDeleteFramebuffers(3,framebuf);
    }
    //  Allocate two textures, two frame buffer objects and a depth buffer
    glGenFramebuffers(3,framebuf);   
    glGenTextures(4,img);
    glGenRenderbuffers(1,&depthbuf);   
    //  Allocate and size texture
    for (int k=0;k<3;k++)
    {
        glBindTexture(GL_TEXTURE_2D,img[k]);
        glTexImage2D(GL_TEXTURE_2D,0,3,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        //  Bind frame buffer to texture
        glBindFramebuffer(GL_FRAMEBUFFER,framebuf[k]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,img[k],0);
        //  Bind depth buffer to frame buffer 0 (and 2?)
        if (k==0 || k==2)
        {
            glBindRenderbuffer(GL_RENDERBUFFER,depthbuf);
            glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthbuf);
        }
    }
    //  Switch back to regular display buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ErrCheck("Framebuffer");
}

// Key pressed callback
void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Discard key releases (keeps PRESS and REPEAT)
    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_RIGHT) {
        r = 0;
        }
        else if (key == GLFW_KEY_LEFT) {
            l = 0;
        }
        else if (key == GLFW_KEY_UP) {
            u = 0;
        }
        else if (key == GLFW_KEY_DOWN) {
            dwn = 0;
        }
        else if (key == GLFW_KEY_W) {
            w = 0;
        }
        else if (key == GLFW_KEY_A) {
            a = 0;
        }
        else if (key == GLFW_KEY_S) {
            s = 0;
        }
        else if (key == GLFW_KEY_D) {
            d = 0;
        }
        else if (key == GLFW_KEY_TAB) {
            tab = 0;
        }
        return;
    }

    // Exit on ESC
    if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, 1);
    // Looking
    else if (key == GLFW_KEY_RIGHT) {
        r = 1;
        th += 5;
    }
    else if (key == GLFW_KEY_LEFT) {
        l = 1;
        th -= 5;
    }
    else if (key == GLFW_KEY_UP) {
        u = 1;
        ph += 5;
    }
    else if (key == GLFW_KEY_DOWN) {
        dwn = 1;
        ph -= 5;
    }
    else if (key == GLFW_KEY_W) {
        w = 1;
    }
    else if (key == GLFW_KEY_A) {
        a = 1;
    }
    else if (key == GLFW_KEY_S) {
        s = 1;
    }
    else if (key == GLFW_KEY_D) {
        d = 1;
    }
    // Zoom
    else if (key == GLFW_KEY_PAGE_DOWN) {
        if (mode != 0) {
            dimA += 0.1;
        }
    }
    else if (key == GLFW_KEY_PAGE_UP) {
        if (mode != 0 && dimA > 1) {
            dimA -= 0.1;
        }
    }
    // Other
    else if (key == GLFW_KEY_TAB) {
        tab = 1;
    }
    else if (key == GLFW_KEY_M) {
        // Change mode
        mode += 1;
        mode %= 3;
    }
    else if (key == GLFW_KEY_R) {
        // Reset cube march 
        currentCube = 0;
    }
    else if (key == GLFW_KEY_Q) {
        // Speed up cube march
        if (cubeStep == 1) {
            cubeStep = CFD;
        }
        else {
            cubeStep = 1;
        }
    }
    else if (key == GLFW_KEY_E) {
        // Skip to the end of cube march
        currentCube = CFD * CFD * CFD;
    }
    //  Wrap angles
    th %= 360;
    ph %= 360;

    // Update projection
    if (mode == 0) {
        Projection(fov, asp, dimB);
    }
    else {
        Projection(fov, asp, dimA);
    }
}

// Put a coin into the world
void placeCoin() {
    // Make sure there is land to put the coin in
    float block = CPFD * CPD;
    float padding = (CUBESAROUND - 1) / 2;
    if (camPos.y < -block * padding) {
        coinPos.x = fmod(rand(), block * padding * 2) - (block * padding) + camPos.x;
        coinPos.y = fmod(rand(), block * padding * 2) - (block * padding) + camPos.y;
        coinPos.z = fmod(rand(), block * padding * 2) - (block * padding) + camPos.z;
        printf("works\n");
    }
    else if (camPos.y < block * padding) {
        coinPos.x = fmod(rand(), block * padding * 2) - (block * padding) + camPos.x;
        coinPos.y = fmod(rand(), block * padding)     - (block * padding);
        coinPos.z = fmod(rand(), block * padding * 2) - (block * padding) + camPos.z;
    }
    else {
        coinPos.x = 10;
        coinPos.y = 10;
        coinPos.z = 10;
    }
    printf("Coin position: %f, %f, %f\n", coinPos.x, coinPos.y, coinPos.z);
}

// Draw a cube's outline
void drawCube(cube c) {
    glPushMatrix();
    SetColor(1, 1, 1);
    glBegin(GL_LINES);
    // Back
    glVertex3f(c.verts[0].x, c.verts[0].y, c.verts[0].z);
    glVertex3f(c.verts[1].x, c.verts[1].y, c.verts[1].z);

    glVertex3f(c.verts[1].x, c.verts[1].y, c.verts[1].z);
    glVertex3f(c.verts[2].x, c.verts[2].y, c.verts[2].z);

    glVertex3f(c.verts[2].x, c.verts[2].y, c.verts[2].z);
    glVertex3f(c.verts[3].x, c.verts[3].y, c.verts[3].z);

    glVertex3f(c.verts[3].x, c.verts[3].y, c.verts[3].z);
    glVertex3f(c.verts[0].x, c.verts[0].y, c.verts[0].z);
    // Front
    glVertex3f(c.verts[4].x, c.verts[4].y, c.verts[4].z);
    glVertex3f(c.verts[5].x, c.verts[5].y, c.verts[5].z);

    glVertex3f(c.verts[5].x, c.verts[5].y, c.verts[5].z);
    glVertex3f(c.verts[6].x, c.verts[6].y, c.verts[6].z);

    glVertex3f(c.verts[6].x, c.verts[6].y, c.verts[6].z);
    glVertex3f(c.verts[7].x, c.verts[7].y, c.verts[7].z);

    glVertex3f(c.verts[7].x, c.verts[7].y, c.verts[7].z);
    glVertex3f(c.verts[4].x, c.verts[4].y, c.verts[4].z);
    // Sides
    glVertex3f(c.verts[0].x, c.verts[0].y, c.verts[0].z);
    glVertex3f(c.verts[4].x, c.verts[4].y, c.verts[4].z);

    glVertex3f(c.verts[1].x, c.verts[1].y, c.verts[1].z);
    glVertex3f(c.verts[5].x, c.verts[5].y, c.verts[5].z);

    glVertex3f(c.verts[2].x, c.verts[2].y, c.verts[2].z);
    glVertex3f(c.verts[6].x, c.verts[6].y, c.verts[6].z);

    glVertex3f(c.verts[3].x, c.verts[3].y, c.verts[3].z);
    glVertex3f(c.verts[7].x, c.verts[7].y, c.verts[7].z);

    glEnd();
    glPopMatrix();
}

// Change a cube's position
void moveCube(cube* c, float x, float y, float z) {
    for (int i = 0; i < 8; i++) {
        c->verts[i].x += x;
        c->verts[i].y += y;
        c->verts[i].z += z;
    }
}

// Make a triangle given only edges
void makeTri(cube c, int e1, int e2, int e3) {
    // Transform edge into 2 indices in vert list
    int i1, i2;
    int edgeToIndex[12][2] = {
        {0, 1}, //0
        {1, 2}, //1
        {2, 3}, //2
        {0, 3}, //3
        {4, 5}, //4
        {5, 6}, //5
        {6, 7}, //6
        {4, 7}, //7
        {0, 4}, //8
        {1, 5}, //9
        {2, 6}, //10
        {3, 7}, //11
    };
    // Get points of triangle by averaging the two points defining an edge
    i1 = edgeToIndex[e1][0];
    i2 = edgeToIndex[e1][1];
    vec3 p1 = {(c.verts[i1].x + c.verts[i2].x) / 2, (c.verts[i1].y + c.verts[i2].y) / 2, (c.verts[i1].z + c.verts[i2].z) / 2};
    i1 = edgeToIndex[e2][0];
    i2 = edgeToIndex[e2][1];
    vec3 p2 = {(c.verts[i1].x + c.verts[i2].x) / 2, (c.verts[i1].y + c.verts[i2].y) / 2, (c.verts[i1].z + c.verts[i2].z) / 2};
    i1 = edgeToIndex[e3][0];
    i2 = edgeToIndex[e3][1];
    vec3 p3 = {(c.verts[i1].x + c.verts[i2].x) / 2, (c.verts[i1].y + c.verts[i2].y) / 2, (c.verts[i1].z + c.verts[i2].z) / 2};
    // printf("coord1: %f, coord2: %f, i2: %d\n", c.verts[i1].y, c.verts[i2].y, i2);
    // Draw the triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(p1.x, p1.y, p1.z);
    glColor3f(0, 1, 0);
    glVertex3f(p2.x, p2.y, p2.z);
    glColor3f(0, 0, 1);
    glVertex3f(p3.x, p3.y, p3.z);
    glEnd();
}

// Make a mesh based on weights
// Source: http://paulbourke.net/geometry/polygonise/
void getMesh(cube c) {
    // Step 1: Get unique number representing which verts have what weight
    int cubeindex = 0;
    if (c.weights[0] < isolevel) cubeindex |= 1;
    if (c.weights[1] < isolevel) cubeindex |= 2;
    if (c.weights[2] < isolevel) cubeindex |= 4;
    if (c.weights[3] < isolevel) cubeindex |= 8;
    if (c.weights[4] < isolevel) cubeindex |= 16;
    if (c.weights[5] < isolevel) cubeindex |= 32;
    if (c.weights[6] < isolevel) cubeindex |= 64;
    if (c.weights[7] < isolevel) cubeindex |= 128;
    // Step 2: Filter out bad cubeindex values
    if (edgeTable[cubeindex] == 0)
        return;
    // Step 3: Make triangles
    //int ntriang = 0;
    for (int i=0; triTable[cubeindex][i]!=-1; i+=3) {
        makeTri(c,
            triTable[cubeindex][i  ],
            triTable[cubeindex][i+1],
            triTable[cubeindex][i+2]);
        //ntriang++;
    }
    return;
}

// Make a point field
void makePoints(float ox, float oy, float oz) {
    vec3 p;
    p.x = CPD / 2.; p.y = p.x; p.z = p.x;
    p.x += ox; p.y += oy; p.z += oz;
    glPointSize(5);
    glBegin(GL_POINTS);
    for (int i = 0; i < CPFD; i++) {
        for (int j = 0; j < CPFD; j++) {
            for (int k = 0; k < CPFD; k++) {

                // Make the point
                glVertex3f(p.x, p.y, p.z);

                p.x += CPD;
            }
            p.x = CPD / 2. + ox;
            p.y += CPD;
        }
        p.x = CPD / 2. + ox;
        p.y = CPD / 2. + oy;
        p.z += CPD;
    }
    glEnd();
}

// Make a bunch of chunks
void makeWorld() {
    // Make the points
    float block = CPFD * CPD;
    float padding = (CUBESAROUND - 1) / 2;
    float ox, oy, oz = 0;
    ox = -1 * ((block * padding) + (block / 2));
    oy = -1 * ((block * padding) + (block / 2));
    oz = -1 * ((block * padding) + (block / 2));
    ox += floor(camPos.x / (block / 2)) * (block / 2);
    oy += floor(camPos.y / (block / 2)) * (block / 2);
    oz += floor(camPos.z / (block / 2)) * (block / 2);

    for (int i = 0; i < CUBESAROUND; i++) {
        for (int j = 0; j < CUBESAROUND; j++) {
            for (int k = 0; k < CUBESAROUND; k++) {
                makePoints(i * CPD * CPFD + ox, j * CPD * CPFD + oy, k * CPD * CPFD + oz);
            }
        }
    }
}

// Display
void display(GLFWwindow* window)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Set up camera and lighting
    if (mode == 0) {
        Projection(fov, asp, dimB);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Movement
        float mov = 1;
        float rot = 4;
        if (w == 1) {
            camPos.x += -1 * mov * Sin(camRot.y) * Cos(camRot.x);
            camPos.y += mov                 * Sin(camRot.x);
            camPos.z += mov * Cos(camRot.y) * Cos(camRot.x);
        } 
        else if (s == 1) {
            camPos.x += -1 * mov * Sin(camRot.y + 180) * Cos(camRot.x);
            camPos.y -= mov                       * Sin(camRot.x);
            camPos.z += mov * Cos(camRot.y + 180) * Cos(camRot.x);
        }
        else if (a == 1) {
            camPos.x += -1 * mov * Sin(camRot.y - 90) * Cos(camRot.x);
            camPos.z += mov * Cos(camRot.y - 90) * Cos(camRot.x);
        }
        else if (d == 1) {
            camPos.x += -1 * mov * Sin(camRot.y + 90) * Cos(camRot.x);
            camPos.z += mov * Cos(camRot.y + 90) * Cos(camRot.x);
        }
        if (u == 1) {
            if (camRot.x < 85) {
                camRot.x += rot;
            }
        }
        else if (dwn == 1) {
            if (camRot.x > -85) {
                camRot.x -= rot;
            }
        }
        else if (l == 1) {
            camRot.y -= rot;
            camRot.y = fmod(camRot.y, 360);
        }
        else if (r == 1) {
            camRot.y += rot;
            camRot.y = fmod(camRot.y, 360);
        }
        double rotX = -1 * Sin(camRot.y) * Cos(camRot.x) + camPos.x;
        double rotY = +1                 * Sin(camRot.x) + camPos.y;
        double rotZ = +1 * Cos(camRot.y) * Cos(camRot.x) + camPos.z;
        gluLookAt(camPos.x,camPos.y,camPos.z, rotX,rotY,rotZ, 0,1,0);

        // Make cool rumble animation
        if (fmod(glfwGetTime(), 80) > 60) {
            zh += 0.05;
            int rVal = rand() % 6;
            if (rVal == 0) {
                camPos.x += 0.1;
            }
            else if (rVal == 1) {
                camPos.x -= 0.1;
            }
            else if (rVal == 2) {
                camPos.y += 0.1;
            }
            else if (rVal == 3) {
                camPos.y -= 0.1;
            }
            else if (rVal == 4) {
                camPos.z += 0.1;
            }
            else {
                camPos.z -= 0.1;
            }
        }
        // // Make cool pulse animation (discarded (it looked bad))
        // if (glfwGetTime() > pulseTime) {
        //     if (pulse == 0) {
        //         if (NP < 5) {
        //             NP += 1;
        //         }
        //         else {
        //             pulse = 1;
        //         }
        //     }
        //     else {
        //         if (NP > 1) {
        //             NP -= 1;
        //         }
        //         else {
        //             pulse = 0;
        //         }
        //     }
        //     pulseTime += 0.1;
        // } 
        // Get coins
        if (pow(camPos.x - coinPos.x, 2) + pow(camPos.y - coinPos.y, 2) + pow(camPos.z - coinPos.z, 2) < 20) {
            placeCoin();
            score += 1;
        }
        // Move coin closer if it gets too far away
        if (pow(camPos.x - coinPos.x, 2) + pow(camPos.y - coinPos.y, 2) + pow(camPos.z - coinPos.z, 2) > 5000) {
            placeCoin();
        }
    }
    else {
        Axes(1);
        View(th, ph, fov, dimA);
    }

    // Center the scene
    if (mode == 0) {
        //glTranslated((-CPFD*CPD / 2), (-CPFD*CPD / 2), (-CPFD*CPD / 2));
    }
    else {
        glTranslated((-CFD*CD / 2), (-CFD*CD / 2), (-CFD*CD / 2));
    }

    // Draw using the CPU for modes 1 and 2
    if (mode == 1 || mode == 2) {
        glDisable(GL_LIGHTING);
        for (int i = 0; i < currentCube; i++) {
            if (mode == 1) {
                drawCube(cf[i]);
            }
            getMesh(cf[i]);
        }
        if (currentCube < CFD * CFD * CFD) {
            currentCube += cubeStep;
        }
    }

    // Draw using the GPU for mode 0
    else if (mode == 0) {
        // Draw only glowing things
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[2]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(bloomShader);

        // Send the shader information
        int id;
        // Send cube point dimension (length of one cube)
        id = glGetUniformLocation(bloomShader, "CPD");
        glUniform1f(id, CPD);
        // Send cube point field dimension (length of all cubes)
        id = glGetUniformLocation(bloomShader, "CPFD");
        glUniform1f(id, CPFD);
        id = glGetUniformLocation(bloomShader, "zh");
        glUniform1f(id, zh);
        id = glGetUniformLocation(bloomShader, "camPos");
        float camPosTemp[3] = {camPos.x, camPos.y, camPos.z};
        glUniform3fv(id, 1, camPosTemp);
        id = glGetUniformLocation(bloomShader, "coinPos");
        float coinPosTemp[3] = {coinPos.x, coinPos.y, coinPos.z};
        glUniform3fv(id, 1, coinPosTemp);
        
        // Source: http://www.icare3d.org/codes-and-projects/codes/opengl_geometry_shader_marching_cubes.html
        unsigned int tex;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0 + 0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        // Integer textures must use nearest filtering mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // We create an integer texture with new GL_EXT_texture_integer formats
        glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA16I_EXT, 16, 256, 0,
        GL_ALPHA_INTEGER_EXT, GL_INT, &triTable);

        glUniform1iARB(glGetUniformLocationARB(bloomShader, "triTableTex"), 0);

        makeWorld();

        // Make coins
        glUseProgram(0);
        glPushMatrix();
        SetColor(0.8,0.8,0);
        Cylinder(coinPos.x, coinPos.y, coinPos.z, 1, 0.1, fmod(90 * glfwGetTime(), 360), 0, 16, 0);
        glPopMatrix();

        // Draw only non-glowing things
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_DEPTH_TEST);

        // Lighting
        //Lighting(0,0,30 , 0.3,0.5,0.5);

        // Use the cool geometry shader
        glUseProgram(shader);

        // Send the shader information
        id = glGetUniformLocation(shader, "CPD");
        glUniform1f(id, CPD);
        id = glGetUniformLocation(shader, "CPFD");
        glUniform1f(id, CPFD);
        id = glGetUniformLocation(shader, "zh");
        glUniform1f(id, zh);
        id = glGetUniformLocation(shader, "camPos");
        glUniform3fv(id, 1, camPosTemp);

        glUniform1iARB(glGetUniformLocationARB(shader, "triTableTex"), 0);

        makeWorld();

        // Do post processing
    
        //  Enable shader
        glUseProgram(bloomShader2);
        //  Set screen resolution uniforms
        int width,height;
        glfwGetWindowSize(window,&width,&height);
        id = glGetUniformLocation(bloomShader2,"dX");
        glUniform1f(id,1.0/width);
        id = glGetUniformLocation(bloomShader2,"dY");
        glUniform1f(id,1.0/height);
        //  Identity projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        //  Disable depth test & Enable textures
        glDisable(GL_DEPTH_TEST);
        //glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        for (int i=0; i<NP; i++) {
            //  Output to alternate framebuffers
            //  Final output is to screen
            if (i%2 == 0) {
                glBindFramebuffer(GL_FRAMEBUFFER,i==NP-1?0:framebuf[1]);
            }
            else {
                glBindFramebuffer(GL_FRAMEBUFFER,i==NP-1?0:framebuf[2]);
            }
            //  Clear the screen
            glClear(GL_COLOR_BUFFER_BIT);
            //  Input image is from the last framebuffer
            int imgLocation = glGetUniformLocation(bloomShader2, "img");
            int emLocation = glGetUniformLocation(bloomShader2, "em");
            glUniform1i(imgLocation, 0);
            glUniform1i(emLocation, 1);
            glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
            if (i%2 == 0) {
                glBindTexture(GL_TEXTURE_2D, img[2]);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, img[1]);
            }
            if (i == NP - 1) {
                glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
                glBindTexture(GL_TEXTURE_2D, img[0]);
            }
            else {
                glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
                glBindTexture(GL_TEXTURE_2D, img[3]);
            }
            //  Redraw the screen
            glBegin(GL_QUADS);
            glTexCoord2f(0,0); glVertex2f(-1,-1);
            glTexCoord2f(0,1); glVertex2f(-1,+1);
            glTexCoord2f(1,1); glVertex2f(+1,+1);
            glTexCoord2f(1,0); glVertex2f(+1,-1);
            glEnd();
        }
    }

    // Help screen
    glUseProgram(0);
    if (tab && mode == 0) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(-1,-1);
        glTexCoord2f(0,1); glVertex2f(-1,+1);
        glTexCoord2f(1,1); glVertex2f(+1,+1);
        glTexCoord2f(1,0); glVertex2f(+1,-1);
        glEnd();
    }

    //  Disable textures and shaders
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);

    // Display parameters
    SetColor(1,1,1);
    if (mode == 0) {
        if (!tab) {
            glWindowPos2i(5,45);
            Print("Score=%d", score);
            glWindowPos2i(5,25);
            Print("Position=%.1f,%.1f,%.1f", camPos.x, camPos.y, camPos.z);
            glWindowPos2i(5,5);
            Print("TAB for instructions");
        }
    }
    else {
        glWindowPos2i(5,5);
        Print("Angle=%d,%d Dim=%.1f", th, ph, dimA);
    }

    ErrCheck("display");
    glFlush();
    glfwSwapBuffers(window);
}