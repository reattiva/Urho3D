//
// Copyright (c) 2008-2013 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Context.h"
#include "Node.h"
#include "ParticleEmitter2D.h"
#include "PropertyList.h"
#include "ResourceCache.h"
#include "scene.h"
#include "SceneEvents.h"
#include "SpriteSheet.h"
#include "Texture2D.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;

// Blend function (Copy from OpenGL)
enum BlendFunction
{
    BF_ZERO = 0,
    BF_ONE = 1,
    BF_SRC_COLOR = 0x0300,
    BF_ONE_MINUS_SRC_COLOR = 0x0301,
    BF_SRC_ALPHA = 0x0302,
    BF_ONE_MINUS_SRC_ALPHA = 0x0303,
    BF_DST_ALPHA = 0x0304,
    BF_ONE_MINUS_DST_ALPHA = 0x0305,
    BF_DST_COLOR = 0x0306,
    BF_ONE_MINUS_DST_COLOR = 0x0307,
    BF_SRC_ALPHA_SATURATE = 0x0308,
};

static const BlendFunction destBlendFuncs[] =
{
    BF_ZERO,
    BF_ONE,
    BF_ZERO,
    BF_ONE_MINUS_SRC_ALPHA,
    BF_ONE,
    BF_ONE_MINUS_SRC_ALPHA,
    BF_DST_ALPHA
};

static const BlendFunction srcBlendFuncs[] =
{
    BF_ONE,
    BF_ONE,
    BF_DST_COLOR,
    BF_SRC_ALPHA,
    BF_SRC_ALPHA,
    BF_ONE,
    BF_ONE_MINUS_DST_ALPHA
};

const char* emitterType2DName[] =
{
    "gravity",
    "radius",
    0
};

ParticleEmitter2D::ParticleEmitter2D(Context* context) : Drawable2D(context),
    duration_(-1.0f),
    maxParticles_(1000),
    particleLifespanMin_(1.0f),
    particleLifespanMax_(1.0f),
    startSizeMin_(10.0f),
    startSizeMax_(10.0f),
    endSizeMin_(10.0f),
    endSizeMax_(10.0f),
    angleMin_(0.0f),
    angleMax_(0.0f),
    startSpinMin_(0.0f),
    startSpinMax_(0.0f),
    endSpinMin_(0.0f),
    endSpinMax_(0.0f),
    startColorMin_(Color::WHITE),
    startColorMax_(Color::WHITE),
    endColorMin_(Color::WHITE),
    endColorMax_(Color::WHITE),
    emitterType_(ET_GRAVITY),
    speedMin_(100.0f),
    speedMax_(100.0f),
    gravity_(0.0f, 0.0f),
    radialAccelerationMin_(0.0f),
    radialAccelerationMax_(0.0f),
    tangentialAccelerationMin_(0.0f),
    tangentialAccelerationMax_(0.0f),
    startRadiusMin_(0.0f),
    startRadiusMax_(0.0f),
    endRadiusMin_(0.0f),
    endRadiusMax_(0.0f),
    rotatePerSecondMin_(0.0f),
    rotatePerSecondMax_(0.0f),
    positionMin_(0.0f, 0.0f),
    positionMax_(0.0f, 0.0f),
    timeToLive_(0.0f),
    emissionRate_(0.0f)
{    
}

ParticleEmitter2D::~ParticleEmitter2D()
{
}

void ParticleEmitter2D::RegisterObject(Context* context)
{
    context->RegisterFactory<ParticleEmitter2D>(GEOMETRY_CATEGORY);

    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Duration", duration_, -1.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_INT, "Max Particles", maxParticles_, 1000, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Particle Lifespan Min", particleLifespanMin_, 1.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Particle Lifespan Max", particleLifespanMax_, 1.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Size Min", startSizeMin_, 10.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Size Max", startSizeMax_, 10.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Size Min", endSizeMin_, 10.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Size Max", endSizeMax_, 10.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Angle Min", angleMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Angle Max", angleMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Spin Min", startSpinMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Spin Max", startSpinMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Spin Min", endSpinMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Spin Max", endSpinMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_COLOR, "Start Color Min", startColorMin_, Color::WHITE, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_COLOR, "Start Color Max", startColorMax_, Color::WHITE, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_COLOR, "End Color Min", endColorMin_, Color::WHITE, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_COLOR, "End Color Max", endColorMax_, Color::WHITE, AM_DEFAULT);
    // ENUM_ACCESSOR_ATTRIBUTE(ParticleEmitter2D, "Emitter Type", GetEmitterType, SetEmitterType, EmitterType2D, emitterType2DName, 0, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Speed Min", speedMin_, 100.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Speed Max", speedMax_, 100.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_VECTOR2, "Gravity", gravity_, Vector2::ZERO, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Radial Acceleration Min", radialAccelerationMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Radial Acceleration Max", radialAccelerationMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Tangential Acceleration Min", tangentialAccelerationMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Tangential Acceleration Max", tangentialAccelerationMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Radius Min", startRadiusMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Start Radius Max", startRadiusMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Radius Min", endRadiusMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "End Radius Max", endRadiusMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Rotate Per Second Min", rotatePerSecondMin_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_FLOAT, "Rotate Per Second Max", rotatePerSecondMax_, 0.0f, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_VECTOR2, "Position Min", positionMin_, Vector2::ZERO, AM_DEFAULT);
    ATTRIBUTE(ParticleEmitter2D, VAR_VECTOR2, "Position Max", positionMax_, Vector2::ZERO, AM_DEFAULT);

    COPY_BASE_ATTRIBUTES(ParticleEmitter2D, Drawable2D);
}

void ParticleEmitter2D::OnSetEnabled()
{
    Drawable2D::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
        else
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
    }
}

void ParticleEmitter2D::Update(const FrameInfo& frame)
{
    float timeStep = frame.timeStep_;
    timeToLive_ += timeStep;

    if (emissionRate_ > 0.0f)
    {
        int emissionCount = (int)(emissionRate_ * timeStep);
        while (particles_.Size() < maxParticles_ && emissionCount > 0)
        {           
            EmitNewParticle();
            --emissionCount;
        }
    }

    const Vector3& worldPos = GetNode()->GetWorldPosition();

    unsigned numParticles = particles_.Size();
    for (unsigned i = 0; i < numParticles; ++i)
    {
        Particle2D& particle = particles_[i];

        particle.timeToLive_ -= timeStep;
        if (particle.timeToLive_ > 0.0f)
        {
            if (emitterType_ == ET_GRAVITY)
            {
                Vector2 radial = particle.position_;
                radial.x_ -= worldPos.x_;
                radial.y_ -= worldPos.y_;
                if (radial.x_ || radial.y_)
                    radial.Normalize();

                Vector2 tangential(-radial.y_, radial.x_);

                radial *= particle.radialAccel_;
                tangential *= particle.tangentialAccel_;

                Vector2 accelaration = gravity_ + radial + tangential;
                accelaration *= timeStep;

                particle.velocityX_ += accelaration.x_;
                particle.velocityY_ += accelaration.y_;

                particle.position_.x_ += particle.velocityX_ * timeStep;
                particle.position_.y_ += particle.velocityY_ * timeStep;
            }
            else
            {	
                particle.angle_ += particle.degreesPerSecond_ * timeStep;
                particle.radius_ += particle.radiusDelta_ * timeStep;

                particle.position_.x_ = worldPos.x_ + Cos(particle.angle_) * particle.radius_;
                particle.position_.y_ = worldPos.y_ + Sin(particle.angle_) * particle.radius_;
            }

            particle.color_ += particle.colorDelta_ * timeStep;

            particle.size_ += particle.sizeDelta_* timeStep;
            particle.size_ = Max(particle.size_, 0.0f);

            particle.rotation_ += particle.rotationDelta_ * timeStep;
        } 
        else 
        {
            --numParticles;
            if (i != numParticles)
                particles_[i--] = particles_[numParticles];
        }
    }

    if (particles_.Size() != numParticles)
        particles_.Resize(numParticles);

    OnMarkedDirty(node_);

    verticesDirty_ = true;
}

void ParticleEmitter2D::UpdateBatches(const FrameInfo& frame)
{
    Drawable2D::UpdateBatches(frame);
    batches_[0].worldTransform_ = &Matrix3x4::IDENTITY;
}

bool ParticleEmitter2D::Load(const String& fileName)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    PropertyList* plist = cache->GetResource<PropertyList>(fileName);
    if (!plist)
        return false;

    const PLDictionary& root = plist->GetRoot();

    int maxParticles = (int)root.GetFloat("maxParticles");
    SetMaxParticles(maxParticles);

    angleMin_ = root.GetFloat("angle");
    angleMax_ = angleMin_ + root.GetFloat("angleVariance");

    duration_ = root.GetFloat("duration");

    // Blend mode
    int blendFuncSource = root.GetInt("blendFuncSource");
    int blendFuncDestination = root.GetInt("blendFuncDestination");
    blendMode_ = BLEND_REPLACE;
    for (int i = 0; i < MAX_BLENDMODES; ++i)
    {
        if (blendFuncSource == srcBlendFuncs[i] && blendFuncDestination == destBlendFuncs[i])
        {
            blendMode_ = (BlendMode)i;
            break;
        }
    }

    float r, g, b, a;
    r = root.GetFloat("startColorRed");
    g = root.GetFloat("startColorGreen");
    b = root.GetFloat("startColorBlue");
    a = root.GetFloat("startColorAlpha");    
    float rv, gv, bv, av;
    rv = root.GetFloat("startColorVarianceRed");
    gv = root.GetFloat("startColorVarianceGreen");
    bv = root.GetFloat("startColorVarianceBlue");
    av = root.GetFloat("startColorVarianceAlpha");
    startColorMin_ = Color(r - rv, g - gv, b - bv, a - av);
    startColorMax_ = Color(r + rv, g + gv, b + bv, a + av);

    r = root.GetFloat("finishColorRed");
    g = root.GetFloat("finishColorGreen");
    b = root.GetFloat("finishColorBlue");
    a = root.GetFloat("finishColorAlpha");    
    rv = root.GetFloat("finishColorVarianceRed");
    gv = root.GetFloat("finishColorVarianceGreen");
    bv = root.GetFloat("finishColorVarianceBlue");
    av = root.GetFloat("finishColorVarianceAlpha");
    endColorMin_ = Color(r - rv, g - gv, b - bv, a - av);
    endColorMax_ = Color(r + rv, g + gv, b + bv, a + av);

    float startParticleSize = root.GetFloat("startParticleSize");
    float startParticleSizeVariance = root.GetFloat("startParticleSizeVariance");
    startSizeMin_ = startParticleSize - startParticleSizeVariance;
    startSizeMax_ = startParticleSize + startParticleSizeVariance;

    float endParticleSize = root.GetFloat("finishParticleSize");
    float endParticleSizeVariance = root.GetFloat("finishParticleSizeVariance");
    endSizeMin_ = endParticleSize - endParticleSizeVariance;
    endSizeMax_ = endParticleSize + endParticleSizeVariance;

    float posX = root.GetFloat("sourcePositionx");
    float posY = root.GetFloat("sourcePositiony");
    float posVarX = root.GetFloat("sourcePositionVariancex");
    float posVarY = root.GetFloat("sourcePositionVariancey");
    positionMin_ = Vector2(posX - posVarX, posY - posVarY);
    positionMax_ = Vector2(posX + posVarX, posY + posVarY);

    float rotationStart = root.GetFloat("rotationStart");
    float rotationStartVariance = root.GetFloat("rotationStartVariance");
    startSpinMin_ = rotationStart - rotationStartVariance;
    startSpinMax_ = rotationStart + rotationStartVariance;

    float rotationEnd = root.GetFloat("rotationEnd");
    float rotationEndVariance = root.GetFloat("rotationEndVariance");
    endSpinMin_ = rotationEnd + rotationEndVariance;
    endSpinMax_ = rotationEnd + rotationEndVariance;

    emitterType_ = (EmitterType2D)(int)root.GetFloat("emitterType");
    if (emitterType_ == ET_GRAVITY)
    {
        gravity_.x_ = root.GetFloat("gravityx");
        gravity_.y_ = root.GetFloat("gravityy");

        float speed = root.GetFloat("speed");
        float speedVariance = root.GetFloat("speedVariance");
        speedMin_ = speed - speedVariance;
        speedMax_ = speed + speedVariance;

        float radialAcceleration = root.GetFloat("radialAcceleration");
        float radialAccelVariance = root.GetFloat("radialAccelVariance");
        radialAccelerationMin_ = radialAcceleration - radialAccelVariance;
        radialAccelerationMax_ = radialAcceleration + radialAccelVariance;

        float tangentialAcceleration = root.GetFloat("tangentialAcceleration");
        float tangentialAccelVariance = root.GetFloat("tangentialAccelVariance");
        tangentialAccelerationMin_ = tangentialAcceleration - tangentialAccelVariance;
        tangentialAccelerationMax_ = tangentialAcceleration + tangentialAccelVariance;
    }
    else
    {
        startRadiusMin_ = root.GetFloat("maxRadius");
        startRadiusMax_ = startRadiusMin_ + root.GetFloat("maxRadiusVariance");

        endRadiusMin_ = endRadiusMax_ = root.GetFloat("minRadius");

        rotatePerSecondMin_ = root.GetFloat("rotatePerSecond");
        rotatePerSecondMax_ = rotatePerSecondMin_ + root.GetFloat("rotatePerSecondVariance");
    }

    float particleLifespan = root.GetFloat("particleLifespan");
    float particleLifespanVariance = root.GetFloat("particleLifespanVariance");
    particleLifespanMin_ = particleLifespan - particleLifespanVariance;
    particleLifespanMax_ = particleLifespan + particleLifespanVariance;

    emissionRate_ = maxParticles / particleLifespan;

    String textureFileName = root.GetString("textureFileName");
    Texture2D* texture = cache->GetResource<Texture2D>(textureFileName);
    if (!texture)
        return false;
    SetTexture(texture);

    return true;
}

void ParticleEmitter2D::SetDuration(float duration)
{
    duration_ = duration;
}

void ParticleEmitter2D::SetMaxParticles(unsigned maxParticles)
{
    maxParticles_ = maxParticles;

    particles_.Reserve(maxParticles);
    vertices_.Reserve(maxParticles * 6);
}

void ParticleEmitter2D::SetMinParticleLifespan(float lifeSpan)
{
    particleLifespanMin_ = lifeSpan;
}

void ParticleEmitter2D::SetMaxParticleLifespan(float lifeSpan)
{
    particleLifespanMax_ = lifeSpan;
}

void ParticleEmitter2D::SetMinStartSize(float size)
{
    startSizeMin_ = size;
}

void ParticleEmitter2D::SetMaxStartSize(float size)
{
    startSizeMax_= size;
}

void ParticleEmitter2D::SetMinEndSize(float size)
{
    endSizeMin_ = size;
}

void ParticleEmitter2D::SetMaxEndSize(float size)
{
    endSizeMax_ = size;
}

void ParticleEmitter2D::SetMinAngle(float angle)
{
    angleMin_ = angle;
}

void ParticleEmitter2D::SetMaxAngle(float angle)
{
    angleMax_ = angle;
}

void ParticleEmitter2D::SetMinStartSpin(float spin)
{
    startSpinMin_ = spin;
}

void ParticleEmitter2D::SetMaxStartSpin(float spin)
{
    startSpinMax_ = spin;
}

void ParticleEmitter2D::SetMinEndSpin(float spin)
{
    endSpinMin_ = spin;
}

void ParticleEmitter2D::SetMaxEndSpin(float spin)
{
    endSpinMax_ = spin;
}

void ParticleEmitter2D::SetMinStartColor(const Color& color)
{
    startColorMin_ = color;
}

void ParticleEmitter2D::SetMaxStartColor(const Color& color)
{
    startColorMax_ = color;
}


void ParticleEmitter2D::SetMinEndColor(const Color& color)
{
    endColorMin_ = color;
}

void ParticleEmitter2D::SetMaxEndColor(const Color& color)
{
    endColorMax_ = color;
}

void ParticleEmitter2D::SetEmitterType(EmitterType2D type)
{
    emitterType_ = type;
}

void ParticleEmitter2D::SetMinSpeed(float speed)
{
    speedMin_ = speed;
}

void ParticleEmitter2D::SetMaxSpeed(float speed)
{
    speedMax_ = speed;
}

void ParticleEmitter2D::SetGravity(const Vector2& gravity)
{
    gravity_ = gravity;
}

void ParticleEmitter2D::SetMinRadialAcceleration(float accel)
{
    radialAccelerationMin_ = accel;
}

void ParticleEmitter2D::SetMaxRadialAcceleration(float accel)
{
    radialAccelerationMax_ = accel;
}

void ParticleEmitter2D::SetMinTangentialAcceleration(float accel)
{
    tangentialAccelerationMin_ = accel;
}

void ParticleEmitter2D::SetMaxTangentialAcceleration(float accel)
{
    tangentialAccelerationMax_ = accel;
}

void ParticleEmitter2D::SetMinStartRadius(float radius)
{
    startRadiusMin_ = radius;
}

void ParticleEmitter2D::SetMaxStartRadius(float radius)
{
    startRadiusMax_ = radius;
}

void ParticleEmitter2D::SetMinEndRadius(float radius)
{
    endRadiusMin_ = radius;
}

void ParticleEmitter2D::SetMaxEndRadius(float radiusVar)
{
    endRadiusMax_ = radiusVar;
}

void ParticleEmitter2D::SetMinRotatePerSecond(float rotatePerSecond)
{
    rotatePerSecondMin_= rotatePerSecond;
}

void ParticleEmitter2D::SetMaxRotatePerSecond(float rotatePerSecond)
{
    rotatePerSecondMax_= rotatePerSecond;
}

void ParticleEmitter2D::OnNodeSet(Node* node)
{
    Drawable2D::OnNodeSet(node);

    if (node)
    {
        Scene* scene = GetScene();
        if (scene && IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
    }
}

void ParticleEmitter2D::OnWorldBoundingBoxUpdate()
{
    Drawable2D::OnWorldBoundingBoxUpdate();
    worldBoundingBox_ = boundingBox_;
}

void ParticleEmitter2D::UpdateVertices()
{
    if (!verticesDirty_)
        return;

    vertices_.Clear();

    Texture* texture = texture_;
    if (spriteSheet_)
        texture = spriteSheet_->GetTexture();

    if (!texture)
        return;

    /*
    V1 --------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0 --------V3
    */
    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    if (spriteFrame_)
    {
        float invTexW = 1.0f / texture->GetWidth();
        float invTexH = 1.0f / texture->GetHeight();

        float leftU = spriteFrame_->x_ * invTexW;
        float topV = spriteFrame_->y_* invTexH;

        if (spriteFrame_->rotated_)
        {
            float rightU = (spriteFrame_->x_ + spriteFrame_->height_) * invTexW;
            float bottomV = (spriteFrame_->y_ + spriteFrame_->width_) * invTexH;

            vertex0.uv_ = Vector2(leftU, topV);
            vertex1.uv_ = Vector2(rightU, topV);
            vertex2.uv_ = Vector2(rightU, bottomV);
            vertex3.uv_ = Vector2(leftU, bottomV);
        }
        else
        {
            float rightU = (spriteFrame_->x_ + spriteFrame_->width_) * invTexW;
            float bottomV = (spriteFrame_->y_ + spriteFrame_->height_) * invTexH;
            vertex0.uv_ = Vector2(leftU, bottomV);
            vertex1.uv_ = Vector2(leftU, topV);
            vertex2.uv_ = Vector2(rightU, topV);
            vertex3.uv_ = Vector2(rightU, bottomV);
        }
    }
    else
    {
        vertex0.uv_ = Vector2(0.0f, 1.0f);
        vertex1.uv_ = Vector2(0.0f, 0.0f);
        vertex2.uv_ = Vector2(1.0f, 0.0f);
        vertex3.uv_ = Vector2(1.0f, 1.0f);
    }

    for (unsigned i = 0; i < particles_.Size(); ++i)
    {
        Particle2D& p = particles_[i];

        float c = Cos(p.rotation_);
        float s = Sin(p.rotation_);
        float add = (c + s) * p.size_ * 0.5f;
        float sub = (c - s) * p.size_ * 0.5f;

        vertex0.position_ = Vector3(p.position_.x_ - sub, p.position_.y_ - add, 0.0f);
        vertex1.position_ = Vector3(p.position_.x_ - add, p.position_.y_ + sub, 0.0f);
        vertex2.position_ = Vector3(p.position_.x_ + sub, p.position_.y_ + add, 0.0f);
        vertex3.position_ = Vector3(p.position_.x_ + add, p.position_.y_ - sub, 0.0f);

        vertex0.color_ = vertex1.color_ = vertex2.color_  = vertex3.color_ = p.color_.ToUInt();

        vertices_.Push(vertex0);
        vertices_.Push(vertex1);
        vertices_.Push(vertex2);

        vertices_.Push(vertex0);
        vertices_.Push(vertex2);
        vertices_.Push(vertex3);
    }

    verticesDirty_ = false;
    geometryDirty_ = true;
}

void ParticleEmitter2D::EmitNewParticle()
{
    particles_.Resize(particles_.Size() + 1);
    Particle2D& particle = particles_.Back();

    particle.timeToLive_ = Lerp(particleLifespanMin_, particleLifespanMax_, Random(1.0f));
    particle.timeToLive_ = Max(particle.timeToLive_, 0.01f);
    float invTimeToLive = 1.0f / particle.timeToLive_;

    const Vector3& worldPos = GetNode()->GetWorldPosition();
    particle.position_.x_ = worldPos.x_ + Lerp(positionMin_.x_, positionMax_.x_, Random(1.0f));
    particle.position_.y_ = worldPos.y_ + Lerp(positionMin_.y_, positionMax_.y_, Random(1.0f));

    particle.color_.r_ = Lerp(startColorMin_.r_, startColorMax_.r_, Random(1.0f));
    particle.color_.g_ = Lerp(startColorMin_.g_, startColorMax_.g_, Random(1.0f));
    particle.color_.b_ = Lerp(startColorMin_.b_, startColorMax_.b_, Random(1.0f));
    particle.color_.a_ = Lerp(startColorMin_.a_, startColorMax_.a_, Random(1.0f));
    particle.color_.Clip(true);

    Color endColor;
    endColor.r_ = Lerp(endColorMin_.r_, endColorMax_.r_, Random(1.0f));
    endColor.g_ = Lerp(endColorMin_.g_, endColorMax_.g_, Random(1.0f));
    endColor.b_ = Lerp(endColorMin_.b_, endColorMax_.b_, Random(1.0f));
    endColor.a_ = Lerp(endColorMin_.a_, endColorMax_.a_, Random(1.0f));
    endColor.Clip(true);

    particle.colorDelta_.r_ = (endColor.r_ - particle.color_.r_) * invTimeToLive;
    particle.colorDelta_.g_ = (endColor.g_ - particle.color_.g_) * invTimeToLive;
    particle.colorDelta_.b_ = (endColor.b_ - particle.color_.b_) * invTimeToLive;
    particle.colorDelta_.a_ = (endColor.a_ - particle.color_.a_) * invTimeToLive;

    particle.size_ = Lerp(startSizeMin_, startSizeMax_, Random(1.0f));
    particle.size_ = Max(particle.size_, 0.0f);
    float endSize = Lerp(endSizeMin_, endSizeMax_, Random(1.0f));
    endSize = Max(endSize, 0.0f);
    particle.sizeDelta_ = (endSize - particle.size_) * invTimeToLive;

    particle.rotation_ = Lerp(startSpinMin_, startSpinMax_, Random(1.0f));
    float endRotation = Lerp(endSpinMin_, endSpinMax_, Random(1.0f));
    particle.rotationDelta_ = (endRotation - particle.rotation_) * invTimeToLive;

    float angle = Lerp(angleMin_, angleMax_, Random(1.0f));

    if (emitterType_ == ET_GRAVITY)
    {
        float speed = Lerp(speedMin_, speedMax_, Random(1.0f));
        particle.velocityX_ = Cos(angle) * speed;
        particle.velocityY_ = Sin(angle) * speed;
        particle.radialAccel_ = Lerp(radialAccelerationMin_, radialAccelerationMax_, Random(1.0f));
        particle.tangentialAccel_ = Lerp(tangentialAccelerationMin_, tangentialAccelerationMax_, Random(1.0f));
    }
    else
    {
        particle.radius_ = Lerp(startRadiusMin_, startRadiusMax_, Random(1.0f));
        float endRadius = Lerp(endRadiusMin_, endRadiusMax_, Random(1.0f));
        particle.radiusDelta_ = (endRadius - particle.radius_) * invTimeToLive;
        particle.angle_ = angle;
        particle.degreesPerSecond_ = Lerp(rotatePerSecondMin_, rotatePerSecondMax_, Random(1.0f));
    }
}

void ParticleEmitter2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    MarkForUpdate();
}

}