
#include "../Core/Context.h"
#include "../Core/Main.h"
#include "../Engine/Engine.h"

#include "SampleGS.h"

int RunApplication()
{
    Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
    Urho3D::SharedPtr<Sample> application;
    int number = 1;
    if (number == 1)
        application = new SampleGS(context);
    if (application)
        return application->Run();
    return 0;
}

URHO3D_DEFINE_MAIN(RunApplication())
