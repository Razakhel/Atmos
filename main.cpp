#include <RaZ/Application.hpp>
#include <RaZ/Math/Transform.hpp>
#include <RaZ/Render/Light.hpp>
#include <RaZ/Render/RenderSystem.hpp>
#include <RaZ/Render/Texture.hpp>

#include <iostream>

using namespace std::literals;
using namespace Raz::Literals;

constexpr Raz::Vec3f earthCenter(0.f);
constexpr float earthRadius = 10.f;
constexpr float atmosphereRadius = 5.f;

int main(int argc, char* argv[]) {
  std::cout << "Program:";
  for (int i = 0; i < argc; ++i)
    std::cout << ' ' << argv[i];
  std::cout << std::endl;

  ////////////////////
  // Initialization //
  ////////////////////

  Raz::Application app;
  Raz::World& world = app.addWorld(3);

  ///////////////
  // Rendering //
  ///////////////

  auto& renderSystem = world.addSystem<Raz::RenderSystem>(1280, 720, "Atmos", 2);

  Raz::RenderPass& geometryPass = renderSystem.getGeometryPass();
  geometryPass.getProgram().setShaders(Raz::VertexShader(RAZ_ROOT + "shaders/common.vert"s),
                                       Raz::FragmentShader(RAZ_ROOT + "shaders/cook-torrance.frag"s));

  Raz::Window& window = renderSystem.getWindow();

#if !defined(USE_OPENGL_ES)
  // Allow wireframe toggling
  bool isWireframe = false;
  window.addKeyCallback(Raz::Keyboard::Z, [&isWireframe] (float /* deltaTime */) {
    isWireframe = !isWireframe;
    Raz::Renderer::setPolygonMode(Raz::FaceOrientation::FRONT_BACK, (isWireframe ? Raz::PolygonMode::LINE : Raz::PolygonMode::FILL));
  }, Raz::Input::ONCE);
#endif

  // Allowing to quit the application with the Escape key
  window.addKeyCallback(Raz::Keyboard::ESCAPE, [&app] (float /* deltaTime */) { app.quit(); });

  /////////////////////
  // Atmosphere pass //
  /////////////////////

  Raz::RenderGraph& renderGraph = renderSystem.getRenderGraph();

  const Raz::Texture& depthBuffer  = renderGraph.addTextureBuffer(window.getWidth(), window.getHeight(), 0, Raz::ImageColorspace::DEPTH);
  const Raz::Texture& colorBuffer  = renderGraph.addTextureBuffer(window.getWidth(), window.getHeight(), 1, Raz::ImageColorspace::RGBA);
  const Raz::Texture& normalBuffer = renderGraph.addTextureBuffer(window.getWidth(), window.getHeight(), 2, Raz::ImageColorspace::RGB);

  geometryPass.addWriteTexture(depthBuffer);
  geometryPass.addWriteTexture(colorBuffer);
  geometryPass.addWriteTexture(normalBuffer);

  Raz::RenderPass& atmospherePass = renderGraph.addNode(Raz::FragmentShader(ATMOS_ROOT + "shaders/atmosphere.frag"s));
  atmospherePass.addReadTexture(depthBuffer, "uniSceneBuffers.depth");
  atmospherePass.addReadTexture(colorBuffer, "uniSceneBuffers.color");
  atmospherePass.addReadTexture(normalBuffer, "uniSceneBuffers.normal");

  geometryPass.addChildren(atmospherePass);

  // Sending information needed for the atmosphere to be rendered
  Raz::ShaderProgram& atmosphereProgram = atmospherePass.getProgram();
  atmosphereProgram.sendUniform("uniEarthCenter", earthCenter);
  atmosphereProgram.sendUniform("uniEarthRadius", earthRadius);
  atmosphereProgram.sendUniform("uniAtmosphereRadius", atmosphereRadius);

  ///////////////////
  // Camera entity //
  ///////////////////

  Raz::Entity& camera = world.addEntity();
  auto& cameraComp    = camera.addComponent<Raz::Camera>(window.getWidth(), window.getHeight());
  auto& cameraTrans   = camera.addComponent<Raz::Transform>(Raz::Vec3f(0.f, 0.f, -20.f));

  ///////////
  // Earth //
  ///////////

  Raz::Entity& mesh = world.addEntity();
  auto& meshComp    = mesh.addComponent<Raz::Mesh>(Raz::Sphere(earthCenter, earthRadius), 50, Raz::SphereMeshType::UV);
  mesh.addComponent<Raz::Transform>();

  auto earthMaterial = Raz::MaterialCookTorrance::create();
  earthMaterial->setAlbedoMap(Raz::Texture::create(ATMOS_ROOT + "assets/textures/earth.png"s, 0));
  earthMaterial->setNormalMap(Raz::Texture::create(ATMOS_ROOT + "assets/textures/earth_normal.png"s, 1));
  earthMaterial->setMetallicMap(Raz::Texture::create(Raz::ColorPreset::BLACK, 2));
  earthMaterial->setRoughnessMap(Raz::Texture::create(Raz::ColorPreset::WHITE, 3));
  earthMaterial->setAmbientOcclusionMap(Raz::Texture::create(Raz::ColorPreset::WHITE, 4));
  meshComp.setMaterial(std::move(earthMaterial));

  /////////
  // Sun //
  /////////

  Raz::Entity& light = world.addEntity();
  light.addComponent<Raz::Light>(Raz::LightType::DIRECTIONAL,             // Type
                                 Raz::Vec3f(0.f, -1.f, -1.f).normalize(), // Direction
                                 1.f,                                     // Energy
                                 Raz::Vec3f(1.f));                        // Color (RGB)
  light.addComponent<Raz::Transform>();

  /////////////////////
  // Camera controls //
  /////////////////////

  float cameraSpeed = 1.f;
  window.addKeyCallback(Raz::Keyboard::LEFT_SHIFT,
                        [&cameraSpeed] (float /* deltaTime */) { cameraSpeed = 2.f; },
                        Raz::Input::ONCE,
                        [&cameraSpeed] () { cameraSpeed = 1.f; });
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

  window.addMouseMoveCallback([&cameraTrans, &window] (double xMove, double yMove) {
    // Dividing move by window size to scale between -1 and 1
    cameraTrans.rotate(-90_deg * yMove / window.getHeight(),
                       -90_deg * xMove / window.getWidth());
  });

  /////////////////////
  // Mouse callbacks //
  /////////////////////

  window.disableCursor(); // Disabling mouse cursor to allow continuous rotations
  window.addKeyCallback(Raz::Keyboard::LEFT_ALT,
                        [&window] (float /* deltaTime */) { window.showCursor(); },
                        Raz::Input::ONCE,
                        [&window] () { window.disableCursor(); });

  //////////////////////////
  // Starting application //
  //////////////////////////

  app.run();

  return EXIT_SUCCESS;
}
