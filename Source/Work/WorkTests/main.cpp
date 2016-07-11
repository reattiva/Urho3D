
#include "../Core/Context.h"
#include "../Core/Main.h"
#include "../Engine/Engine.h"

#include "SampleGS.h"
#include "SampleCS.h"
#include "SampleCS2.h"
#include "SampleRWBUF.h"
#include "SampleTA.h"
#include "SampleTARW.h"
#include "SampleScene.h"

int getNumber()
{
    return 7;
}

int RunApplication()
{
    Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
    Urho3D::SharedPtr<Sample> application;

    switch (getNumber())
    {
    case 1:
        application = new SampleGS(context);
        break;
    case 2:
        application = new SampleCS(context);
        break;
    case 3:
        application = new SampleCS2(context);
        break;
    case 4:
        application = new SampleRWBUF(context);
        break;
    case 5:
        application = new SampleTA(context);
        break;
    case 6:
        application = new SampleTARW(context);
        break;
    case 7:
        application = new SampleScene(context);
        break;
    }

    if (application)
        return application->Run();
    return 0;
}

URHO3D_DEFINE_MAIN(RunApplication())
