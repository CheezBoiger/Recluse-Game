#include "Game/Engine.hpp"


using namespace Recluse;


void Controller()
{
  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
}

int main(int c, char* argv[])
{
  gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800);
  gEngine().SetControlInput(Controller);
  Window* window = gEngine().GetWindow();
  window->Show();

  gEngine().Run();
  gRenderer().Build();
  while (gEngine().Running()) {
    Time::Update();

    gEngine().Update();
    gEngine().ProcessInput();
  }
  
  gEngine().CleanUp();
  return 0;
}