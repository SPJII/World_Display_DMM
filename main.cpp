#include <iostream> 
#include <SDL.h>
#include <SDL_image.h>
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdlib>
#include <ctime>
#include <cmath> // For trigonometric functions

// Screen dimensions
const int SCREEN_WIDTH = 1915;
const int SCREEN_HEIGHT = 1030;

// Zoom limits
const float MIN_ZOOM = 2.1f;
const float MAX_ZOOM = 20.0f;

// Timing constants
const Uint32 RETURN_TO_ORIGINAL_DELAY = 2000; // 2 seconds delay for returning to original rotation
Uint32 lastInteractionTime = 0;  // Track the last time the user interacted

// Base class for celestial bodies
class CelestialBody {
public:
    virtual void render() = 0;  // Polymorphic render method
    virtual void update() = 0;  // Polymorphic update method
    virtual ~CelestialBody() {} // Virtual destructor for proper cleanup
};

// Forward declaration of Moon class
class Moon;

// Moon class (inherits from CelestialBody)
class Moon : public CelestialBody {
protected:
    float orbitAngle;
    float distance;
    float size;
    GLuint textureID, atmosphereTextureID;

public:
    Moon(float d, float s, GLuint texture, GLuint atmosphereTexture)
        : distance(d), size(s), textureID(texture), atmosphereTextureID(atmosphereTexture), orbitAngle(0.0f) {}

    virtual void render() override {
        // The moon's position is now relative to the planet's coordinate system
        glPushMatrix();
        glRotatef(orbitAngle, 0.0f, 1.0f, 0.0f); // Orbit around the Y-axis
        glTranslatef(distance, 0.0f, 0.0f);        // Move the moon out along the X-axis

        // Render the moon
        glBindTexture(GL_TEXTURE_2D, textureID);
        renderSphere(size, 30, 30);

        // Render the moon's atmosphere
        glPushMatrix();
        glRotatef(orbitAngle * 0.5f, 0.0f, 1.0f, 0.0f); // Atmosphere rotates slower
        glBindTexture(GL_TEXTURE_2D, atmosphereTextureID);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        renderSphere(size + 0.05f, 30, 30);  // Slightly larger for atmosphere
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glPopMatrix();

        glPopMatrix();
    }

    virtual void update() override {
        orbitAngle += 0.5f;  // Adjust speed as necessary
        if (orbitAngle >= 360.0f) orbitAngle -= 360.0f;
    }

    static void renderSphere(float radius, int slices, int stacks) {
        GLUquadric* quadric = gluNewQuadric();
        gluQuadricTexture(quadric, GL_TRUE);
        gluSphere(quadric, radius, slices, stacks);
        gluDeleteQuadric(quadric);
    }
};

// Planet class
class Planet : public CelestialBody {
protected:
    float rotationX, rotationY, zoom;
    float passiveRotationSpeed;
    GLuint textureID, atmosphereTextureID;
    float radius, atmosphereRadius;
    Moon* moon; // Pointer to the moon

    // Separate rotation variables for user interaction and passive rotation
    float userRotationX, userRotationY;

    // Orbit variables
    float orbitRadius;
    float orbitAngle;
    float orbitSpeed;
public:
    float positionX, positionZ; // Made public to access in main function

    Planet(float r, float atmosphereR, GLuint texture, GLuint atmosphereTexture, Moon* m,
        float orbitR, float orbitS)
        : radius(r), atmosphereRadius(atmosphereR), textureID(texture), atmosphereTextureID(atmosphereTexture),
        rotationX(0.0f), rotationY(0.0f), zoom(5.0f), passiveRotationSpeed(0.1f), moon(m),
        userRotationX(0.0f), userRotationY(0.0f),
        orbitRadius(orbitR), orbitAngle(0.0f), orbitSpeed(orbitS), positionX(orbitR), positionZ(0.0f)
    {
    }

    // Getter methods to access protected members
    float getRotationX() const { return rotationX; }
    float getRotationY() const { return rotationY; }
    float getZoom() const { return zoom; }

    void setRotation(float rotX, float rotY) {
        userRotationX = rotX;
        userRotationY = rotY;
    }

    void setZoom(float z) { zoom = z; }

    // Update the planet rotation passively and reset X-axis after interaction
    virtual void update() override {
        Uint32 currentTime = SDL_GetTicks();
        // Passive rotation to the right (Y-axis)
        rotationY += passiveRotationSpeed;
        if (rotationY >= 360.0f) rotationY -= 360.0f;

        // Update orbit angle
        orbitAngle += orbitSpeed;
        if (orbitAngle >= 360.0f) orbitAngle -= 360.0f;

        // Update position
        positionX = orbitRadius * cosf(orbitAngle * M_PI / 180.0f);
        positionZ = orbitRadius * sinf(orbitAngle * M_PI / 180.0f);

        // Reset X-axis to 0 after 2 seconds of no interaction
        if (currentTime - lastInteractionTime >= RETURN_TO_ORIGINAL_DELAY && userRotationX != 0.0f) {
            if (userRotationX > 0.0f) userRotationX -= 0.5f;
            if (userRotationX < 0.0f) userRotationX += 0.5f;
            if (fabs(userRotationX) < 0.5f) userRotationX = 0.0f; // Snap to zero
        }

        // Update moon
        if (moon) {
            moon->update();
        }
    }

    virtual void render() override {
        // Render the planet
        glPushMatrix();
        glTranslatef(positionX, 0.0f, positionZ);
        glRotatef(userRotationX, 1.0f, 0.0f, 0.0f); // User-controlled rotation
        glRotatef(userRotationY, 0.0f, 1.0f, 0.0f); // User-controlled rotation
        glRotatef(rotationY, 0.0f, 1.0f, 0.0f);     // Passive rotation

        glBindTexture(GL_TEXTURE_2D, textureID);
        renderSphere(radius, 40, 40);

        // Render the atmosphere
        glPushMatrix();
        glRotatef(rotationY + 5.0f, 0.0f, 1.0f, 0.0f);  // Atmosphere rotates based on passive rotation
        glBindTexture(GL_TEXTURE_2D, atmosphereTextureID);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);  // Set translucency
        renderSphere(atmosphereRadius, 40, 40);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Reset opacity
        glPopMatrix();

        // Render the moon relative to the planet
        if (moon) {
            moon->render();
        }

        glPopMatrix();
    }

    static void renderSphere(float radius, int slices, int stacks) {
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* quadric = gluNewQuadric();
        gluQuadricTexture(quadric, GL_TRUE);
        gluSphere(quadric, radius, slices, stacks);
        gluDeleteQuadric(quadric);
        glPopMatrix();
    }
};

// Sun class (inherits from CelestialBody)
class Sun : public CelestialBody {
protected:
    float radius;
    GLuint textureID;

public:
    Sun(float r, GLuint texture)
        : radius(r), textureID(texture) {}

    virtual void render() override {
        glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, textureID);
        Planet::renderSphere(radius, 40, 40);
        glPopMatrix();
    }

    virtual void update() override {
        // Sun doesn't need to update
    }
};

// Mouse controls and variables
float sphereRotationY = 0.0f;
float sphereRotationX = 0.0f;
bool dragging = false;
int lastMouseX, lastMouseY;

// Function prototypes
void initSDL(SDL_Window*& window, SDL_GLContext& context);
void initOpenGL();
GLuint loadTexture(const char* filename);
void handleInput(SDL_Event& event, bool& running, Planet& planet);
void cleanup(SDL_Window* window, SDL_GLContext context);

// Main function
int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_GLContext context;

    // Initialize SDL and OpenGL
    initSDL(window, context);
    initOpenGL();

    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        cleanup(window, context);
        return 1;
    }

    // Load textures
    GLuint planetTexture = loadTexture("map2.png");
    GLuint planetAtmosphereTexture = loadTexture("clouds.png");
    GLuint moonTexture = loadTexture("moon.jpg");
    GLuint moonAtmosphereTexture = loadTexture("clouds.png");
    GLuint sunTexture = loadTexture("map2.png"); // Add sun texture

    // Create moon object first with realistic distance and size
    Moon* moon = new Moon(5.0f, 0.27f, moonTexture, moonAtmosphereTexture); // Distance increased to 5.0f and size to 0.27f

    // Create planet object and pass the moon to it, with orbit around the sun
    Planet planet(1.0f, 1.05f, planetTexture, planetAtmosphereTexture, moon, 20.0f, 0.1f); // orbit radius 20 units, orbit speed 0.1f degrees per frame

    // Create sun object
    Sun sun(10.0f, sunTexture); // Sun radius is 10 units

    bool running = true;
    SDL_Event event;

    // Main loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            handleInput(event, running, planet);
        }

        // Update celestial bodies
        planet.update();
        sun.update(); // Although sun doesn't need updating, included for consistency

        // Clear the screen and set the background color to black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // Set light position at sun's position
        GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

        // Set camera to focus on planet
        gluLookAt(planet.positionX, 0.0f, planet.positionZ + planet.getZoom(),
            planet.positionX, 0.0f, planet.positionZ,
            0.0f, 1.0f, 0.0f);

        // Render celestial objects
        sun.render();
        planet.render();

        // Swap buffers (double buffering)
        SDL_GL_SwapWindow(window);
    }

    // Clean up
    delete moon; // Free the moon object
    IMG_Quit();
    cleanup(window, context);
    return 0;
}

// SDL Initialization
void initSDL(SDL_Window*& window, SDL_GLContext& context) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Set OpenGL version (using 2.1 for compatibility)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("3D Planet and Moon with Atmospheres",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Enable VSync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        std::cerr << "Warning: Unable to set VSync! SDL_Error: " << SDL_GetError() << std::endl;
    }
}

// OpenGL Initialization
void initOpenGL() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)SCREEN_WIDTH / (double)SCREEN_HEIGHT, 1.0, 1000.0); // Increased far clipping plane

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Light at sun's position
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Handle input
void handleInput(SDL_Event& event, bool& running, Planet& planet) {
    switch (event.type) {
    case SDL_QUIT:
        running = false;
        break;
    case SDL_MOUSEMOTION:
        if (dragging) {
            int dx = event.motion.x - lastMouseX;
            int dy = event.motion.y - lastMouseY;

            sphereRotationY += dx * 0.5f;
            sphereRotationX -= dy * 0.5f;

            // Clamp the X-axis rotation to prevent flipping
            if (sphereRotationX > 40.0f) sphereRotationX = 40.0f;
            if (sphereRotationX < -40.0f) sphereRotationX = -40.0f;

            lastMouseX = event.motion.x;
            lastMouseY = event.motion.y;

            planet.setRotation(sphereRotationX, sphereRotationY);
            lastInteractionTime = SDL_GetTicks();  // Update the last interaction time
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            dragging = true;
            lastMouseX = event.button.x;
            lastMouseY = event.button.y;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            dragging = false;
        }
        break;
    case SDL_MOUSEWHEEL:
        if (event.wheel.y > 0) {
            planet.setZoom(planet.getZoom() - 0.5f); // Zoom in
        }
        else if (event.wheel.y < 0) {
            planet.setZoom(planet.getZoom() + 0.5f); // Zoom out
        }
        if (planet.getZoom() < MIN_ZOOM) planet.setZoom(MIN_ZOOM);
        if (planet.getZoom() > MAX_ZOOM) planet.setZoom(MAX_ZOOM);
        break;
    }
}

// Load texture function
GLuint loadTexture(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        std::cerr << "Failed to load texture (" << filename << "): " << IMG_GetError() << std::endl;
        exit(1);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    // Horizontal wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);    // Vertical wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Minification filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Magnification filter

    // Determine the format
    GLenum format;
    if (surface->format->BytesPerPixel == 3) {
        format = GL_RGB;
    }
    else if (surface->format->BytesPerPixel == 4) {
        format = GL_RGBA;
    }
    else {
        std::cerr << "Unsupported image format for texture: " << filename << std::endl;
        SDL_FreeSurface(surface);
        exit(1);
    }

    // Upload the texture to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_FreeSurface(surface);
    return textureID;
}

// Cleanup resources
void cleanup(SDL_Window* window, SDL_GLContext context) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}