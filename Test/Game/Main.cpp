#include "Game/Engine.hpp"


using namespace Recluse;

int main(int c, char* argv[])
{
  gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800);
  gEngine().CleanUp();
  return 0;
}