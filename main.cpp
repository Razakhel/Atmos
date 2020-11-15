#include <iostream>

int main(int argc, char* argv[]) {
#if defined(ATMOS_CONFIG_DEBUG)
  std::cout << "[Atmos] Debug" << std::endl;
#elif defined(ATMOS_CONFIG_RELEASE)
  std::cout << "[Atmos] Release" << std::endl;
#endif

  std::cout << "Program:";
  for (int i = 0; i < argc; ++i)
    std::cout << ' ' << argv[i];
  std::cout << std::endl;

  return 0;
}
