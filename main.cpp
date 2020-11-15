#include <RaZ/Application.hpp>
#include <RaZ/Math/Transform.hpp>
#include <RaZ/Render/Light.hpp>
#include <RaZ/Render/RenderSystem.hpp>
#include <RaZ/Render/Texture.hpp>

#include <iostream>

using namespace std::literals;
using namespace Raz::Literals;

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
  auto& meshComp    = mesh.addComponent<Raz::Mesh>(Raz::Sphere(Raz::Vec3f(0.f), 10.f), 50, Raz::SphereMeshType::UV);
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

  //////////////////////////
  // Starting application //
  //////////////////////////

  app.run();

  return EXIT_SUCCESS;
}
