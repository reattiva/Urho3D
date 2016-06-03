
#include "../Core/Context.h"
#include "../Core/Main.h"
#include "../Engine/Engine.h"

#include "SampleGS.h"
#include "SampleCS.h"
#include "SampleCS2.h"

int getNumber()
{
    return 1;
}

int RunApplication()
{
    Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
    Urho3D::SharedPtr<Sample> application;
    int number = getNumber();
    if (number == 1)
        application = new SampleGS(context);
    else if (number == 2)
        application = new SampleCS(context);
    else if (number == 3)
        application = new SampleCS2(context);
    if (application)
        return application->Run();
    return 0;
}

URHO3D_DEFINE_MAIN(RunApplication())
