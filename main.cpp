#include <RaZ/Application.hpp>
#include <RaZ/Data/Mesh.hpp>
#include <RaZ/Math/Angle.hpp>
#include <RaZ/Math/Quaternion.hpp>
#include <RaZ/Math/Transform.hpp>
#include <RaZ/Render/Light.hpp>
#include <RaZ/Render/MeshRenderer.hpp>
#include <RaZ/Render/RenderSystem.hpp>
#include <RaZ/Render/Texture.hpp>
#include <RaZ/Utils/Logger.hpp>

using namespace std::literals;
using namespace Raz::Literals;

namespace {

constexpr Raz::Vec3f earthCenter      = Raz::Vec3f(0.f);
constexpr float earthRadius           = 15.f;
constexpr float atmosphereRadius      = 15.f;
const Raz::Vec3f sunDir               = Raz::Vec3f(0.f, -1.f, 1.f).normalize();
constexpr int scatterPointCount       = 10;
constexpr int opticalDepthSampleCount = 10;
constexpr float densityFalloff        = 10.f;
Raz::Vec3f colorWavelengths           = Raz::Vec3f(700.f, 530.f, 440.f);
float scatteringStrength              = 1.f;

inline Raz::Vec3f computeScatteringCoeffs() {
  float redScattering = 400.f / colorWavelengths.x();
  redScattering      *= redScattering; // Squared
  redScattering      *= redScattering; // Fourth
  redScattering      *= scatteringStrength;

  float greenScattering = 400.f / colorWavelengths.y();
  greenScattering      *= greenScattering;
  greenScattering      *= greenScattering;
  greenScattering      *= scatteringStrength;

  float blueScattering = 400.f / colorWavelengths.z();
  blueScattering      *= blueScattering;
  blueScattering      *= blueScattering;
  blueScattering      *= scatteringStrength;

  return Raz::Vec3f(redScattering, greenScattering, blueScattering);
}

} // namespace

int main() {
  try {
    ////////////////////
    // Initialization //
    ////////////////////

    Raz::Application app;
    Raz::World& world = app.addWorld(3);

    Raz::Logger::setLoggingLevel(Raz::LoggingLevel::ALL);

    ///////////////
    // Rendering //
    ///////////////

    auto& renderSystem = world.addSystem<Raz::RenderSystem>(1280u, 720u, "Atmos", Raz::WindowSetting::DEFAULT, 2);

    Raz::RenderPass& geometryPass = renderSystem.getGeometryPass();
    geometryPass.getProgram().setShaders(Raz::VertexShader(RAZ_ROOT + "shaders/common.vert"s),
                                         Raz::FragmentShader(RAZ_ROOT + "shaders/cook-torrance.frag"s));

    renderSystem.setCubemap(Raz::Cubemap(ATMOS_ROOT + "assets/skyboxes/space_right.png"s, ATMOS_ROOT + "assets/skyboxes/space_left.png"s,
                                         ATMOS_ROOT + "assets/skyboxes/space_up.png"s,    ATMOS_ROOT + "assets/skyboxes/space_down.png"s,
                                         ATMOS_ROOT + "assets/skyboxes/space_front.png"s, ATMOS_ROOT + "assets/skyboxes/space_back.png"s));

    Raz::Window& window = renderSystem.getWindow();

    // Allowing to quit the application with the Escape key
    window.addKeyCallback(Raz::Keyboard::ESCAPE, [&app] (float /* deltaTime */) noexcept { app.quit(); });

    /////////////////////
    // Atmosphere pass //
    /////////////////////

    Raz::RenderGraph& renderGraph = renderSystem.getRenderGraph();

    const Raz::Texture& depthBuffer  = renderGraph.addTextureBuffer(window.getWidth(), window.getHeight(), Raz::ImageColorspace::DEPTH);
    const Raz::Texture& colorBuffer  = renderGraph.addTextureBuffer(window.getWidth(), window.getHeight(), Raz::ImageColorspace::RGBA);

    geometryPass.addWriteTexture(depthBuffer);
    geometryPass.addWriteTexture(colorBuffer);

    Raz::RenderPass& atmospherePass = renderGraph.addNode(Raz::FragmentShader(ATMOS_ROOT + "shaders/atmosphere.frag"s));
    atmospherePass.addReadTexture(depthBuffer, "uniSceneBuffers.depth");
    atmospherePass.addReadTexture(colorBuffer, "uniSceneBuffers.color");

    geometryPass.addChildren(atmospherePass);

    // Sending information needed for the atmosphere to be rendered
    Raz::ShaderProgram& atmosphereProgram = atmospherePass.getProgram();
    atmosphereProgram.sendUniform("uniEarthCenter", earthCenter);
    atmosphereProgram.sendUniform("uniEarthRadius", earthRadius);
    atmosphereProgram.sendUniform("uniAtmosphereRadius", atmosphereRadius);
    atmosphereProgram.sendUniform("uniDirToSun", -sunDir);
    atmosphereProgram.sendUniform("uniScatterPointCount", scatterPointCount);
    atmosphereProgram.sendUniform("uniOpticalDepthSampleCount", opticalDepthSampleCount);
    atmosphereProgram.sendUniform("uniDensityFalloff", densityFalloff);
    atmosphereProgram.sendUniform("uniScatteringCoeffs", computeScatteringCoeffs());

    ///////////////////
    // Camera entity //
    ///////////////////

    Raz::Entity& camera = world.addEntity();
    auto& cameraComp    = camera.addComponent<Raz::Camera>(window.getWidth(), window.getHeight());
    auto& cameraTrans   = camera.addComponent<Raz::Transform>(Raz::Vec3f(-17.5f, 5.f, -60.f));

    ///////////
    // Earth //
    ///////////

    Raz::Entity& earth = world.addEntity();
    earth.addComponent<Raz::Transform>();
    auto& mesh         = earth.addComponent<Raz::Mesh>(Raz::Sphere(earthCenter, earthRadius), 100, Raz::SphereMeshType::UV);
    auto& meshRenderer = earth.addComponent<Raz::MeshRenderer>(mesh);

    auto material = Raz::MaterialCookTorrance::create(Raz::Vec3f(1.f), 0.f, 0.f);
    material->setAlbedoMap(Raz::Texture::create(ATMOS_ROOT + "assets/textures/earth.png"s, 0));
    material->setNormalMap(Raz::Texture::create(ATMOS_ROOT + "assets/textures/earth_normal.png"s, 1));
    meshRenderer.setMaterial(std::move(material));

    /////////
    // Sun //
    /////////

    Raz::Entity& light = world.addEntity();
    auto& lightComp = light.addComponent<Raz::Light>(Raz::LightType::DIRECTIONAL, // Type
                                                     sunDir,                      // Direction
                                                     1.f,                         // Energy
                                                     Raz::Vec3f(1.f));            // Color (RGB)
    light.addComponent<Raz::Transform>();

    /////////////////////
    // Camera controls //
    /////////////////////

    float cameraSpeed = 1.f;
    window.addKeyCallback(Raz::Keyboard::LEFT_SHIFT,
                          [&cameraSpeed] (float /* deltaTime */) noexcept { cameraSpeed = 2.f; },
                          Raz::Input::ONCE,
                          [&cameraSpeed] () noexcept { cameraSpeed = 1.f; });
    window.addKeyCallback(Raz::Keyboard::SPACE, [&cameraTrans, &cameraSpeed] (float deltaTime) {
      cameraTrans.move(0.f, (10.f * deltaTime) * cameraSpeed, 0.f);
    });
    window.addKeyCallback(Raz::Keyboard::V, [&cameraTrans, &cameraSpeed] (float deltaTime) {
      cameraTrans.move(0.f, (-10.f * deltaTime) * cameraSpeed, 0.f);
    });
    window.addKeyCallback(Raz::Keyboard::W, [&cameraTrans, &cameraComp, &cameraSpeed] (float deltaTime) {
      const float moveVal = (10.f * deltaTime) * cameraSpeed;

      cameraTrans.move(0.f, 0.f, moveVal);
      cameraComp.setOrthoBoundX(cameraComp.getOrthoBoundX() - moveVal);
      cameraComp.setOrthoBoundY(cameraComp.getOrthoBoundY() - moveVal);
    });
    window.addKeyCallback(Raz::Keyboard::S, [&cameraTrans, &cameraComp, &cameraSpeed] (float deltaTime) {
      const float moveVal = (-10.f * deltaTime) * cameraSpeed;

      cameraTrans.move(0.f, 0.f, moveVal);
      cameraComp.setOrthoBoundX(cameraComp.getOrthoBoundX() - moveVal);
      cameraComp.setOrthoBoundY(cameraComp.getOrthoBoundY() - moveVal);
    });
    window.addKeyCallback(Raz::Keyboard::A, [&cameraTrans, &cameraSpeed] (float deltaTime) {
      cameraTrans.move((-10.f * deltaTime) * cameraSpeed, 0.f, 0.f);
    });
    window.addKeyCallback(Raz::Keyboard::D, [&cameraTrans, &cameraSpeed] (float deltaTime) {
      cameraTrans.move((10.f * deltaTime) * cameraSpeed, 0.f, 0.f);
    });

    window.addMouseScrollCallback([&cameraComp] (double /* xOffset */, double yOffset) {
      const float newFov = Raz::Degreesf(cameraComp.getFieldOfView()).value + static_cast<float>(-yOffset) * 2.f;
      cameraComp.setFieldOfView(Raz::Degreesf(std::clamp(newFov, 15.f, 90.f)));
    });

    bool cameraLocked = true; // To allow moving the camera using the mouse

    window.addMouseButtonCallback(Raz::Mouse::RIGHT_CLICK, [&cameraLocked, &window] (float) {
      cameraLocked = false;
      window.changeCursorState(Raz::Cursor::DISABLED);
    }, Raz::Input::ONCE, [&cameraLocked, &window] () {
      cameraLocked = true;
      window.changeCursorState(Raz::Cursor::NORMAL);
    });

    window.addMouseMoveCallback([&cameraLocked, &cameraTrans, &window] (double xMove, double yMove) {
      if (cameraLocked)
        return;

      // Dividing move by window size to scale between -1 and 1
      cameraTrans.rotate(-90_deg * yMove / window.getHeight(),
                         -90_deg * xMove / window.getWidth());
    });

    /////////////
    // Overlay //
    /////////////

    Raz::OverlayWindow& overlayWindow = window.addOverlayWindow("Atmos");

    overlayWindow.addLabel("Press WASD to fly the camera around,");
    overlayWindow.addLabel("Space/V to go up/down,");
    overlayWindow.addLabel("& Shift to move faster.");
    overlayWindow.addLabel("Hold the right mouse button to rotate the camera.");

    overlayWindow.addSeparator();

    bool rotateSun = true;
    overlayWindow.addCheckbox("Enable sun rotation", [&rotateSun] () noexcept { rotateSun = true; }, [&rotateSun] () noexcept { rotateSun = false; }, true);

    overlayWindow.addSeparator();

    overlayWindow.addSlider("Atmosphere radius", [&atmosphereProgram] (float value) {
      atmosphereProgram.sendUniform("uniAtmosphereRadius", value);
    }, earthRadius, earthRadius * 2.f, earthRadius);

    overlayWindow.addSlider("Scatter point count", [&atmosphereProgram] (float value) {
      atmosphereProgram.sendUniform("uniScatterPointCount", static_cast<int>(value));
    }, 0, 20, scatterPointCount);
    overlayWindow.addSlider("Optical depth sample count", [&atmosphereProgram] (float value) {
      atmosphereProgram.sendUniform("uniOpticalDepthSampleCount", static_cast<int>(value));
    }, 0, 20, opticalDepthSampleCount);

    overlayWindow.addSlider("Density falloff", [&atmosphereProgram] (float value) {
      atmosphereProgram.sendUniform("uniDensityFalloff", value);
    }, 0.f, 10.f, densityFalloff);

    overlayWindow.addSlider("Red wavelength", [&atmosphereProgram] (float value) {
      colorWavelengths.x() = value;
      atmosphereProgram.sendUniform("uniScatteringCoeffs", computeScatteringCoeffs());
    }, 400.f, 700.f, colorWavelengths.x());
    overlayWindow.addSlider("Green wavelength", [&atmosphereProgram] (float value) {
      colorWavelengths.y() = value;
      atmosphereProgram.sendUniform("uniScatteringCoeffs", computeScatteringCoeffs());
    }, 400.f, 700.f, colorWavelengths.y());
    overlayWindow.addSlider("Blue wavelength", [&atmosphereProgram] (float value) {
      colorWavelengths.z() = value;
      atmosphereProgram.sendUniform("uniScatteringCoeffs", computeScatteringCoeffs());
    }, 400.f, 700.f, colorWavelengths.z());

    overlayWindow.addSlider("Scattering strength", [&atmosphereProgram] (float value) {
      scatteringStrength = value;
      atmosphereProgram.sendUniform("uniScatteringCoeffs", computeScatteringCoeffs());
    }, 0.f, 10.f, scatteringStrength);

    overlayWindow.addSeparator();

    overlayWindow.addFrameTime("Frame time: %.3f ms/frame"); // Frame time's & FPS counter's texts must be formatted
    overlayWindow.addFpsCounter("FPS: %.1f");

    //////////////////////////
    // Starting application //
    //////////////////////////

    app.run([&rotateSun, &app, &renderSystem, &lightComp, &atmosphereProgram] () {
      if (!rotateSun)
        return;

      const Raz::Mat3f rotation(Raz::Quaternionf(45_deg * app.getDeltaTime(), Raz::Vec3f(-1.f).normalize()).computeMatrix());
      lightComp.setDirection((lightComp.getDirection() * rotation).normalize());
      atmosphereProgram.sendUniform("uniDirToSun", -lightComp.getDirection());
      renderSystem.updateLights();
    });
  } catch (const std::exception& exception) {
    Raz::Logger::error(exception.what());
  }

  return EXIT_SUCCESS;
}
