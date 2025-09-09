#include "src/Core/Application.h"

#include "src/Client/EditorLayer.h"

namespace Mc
{

    class Client : public Application
    {
    public:
        Client(const ApplicationSpecification &spec)
            : Application(spec)
        {
            PushLayer(new EditorLayer());
        }
    };

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "Client";
        spec.CommandLineArgs = args;

        return new Client(spec);
    }

}