#include "src/Core/Base.h"
#include "src/Core/Application.h"


extern Mc::Application *Mc::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char **argv)
{
    Mc::Log::Init();
    auto app = Mc::CreateApplication({argc, argv});
    app->Run();
    delete app;
}