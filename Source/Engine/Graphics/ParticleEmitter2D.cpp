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
#include "ResourceCache.h"
#include "Context.h"
#include "Node.h"
#include "ResourceCache.h"
#include "ParticleEmitter2D.h"
#include "PropertyList.h"
#include "scene.h"
#include "SceneEvents.h"
#include "Texture2D.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];
extern const char* GEOMETRY_CATEGORY;

#define P2D_ZERO                           0
#define P2D_ONE                            1
#define P2D_SRC_COLOR                      0x0300
#define P2D_ONE_MINUS_SRC_COLOR            0x0301
#define P2D_SRC_ALPHA                      0x0302
#define P2D_ONE_MINUS_SRC_ALPHA            0x0303
#define P2D_DST_ALPHA                      0x0304
#define P2D_ONE_MINUS_DST_ALPHA            0x0305
#define P2D_DST_COLOR                      0x0306
#define P2D_ONE_MINUS_DST_COLOR            0x0307
#define P2D_SRC_ALPHA_SATURATE             0x0308

static const unsigned p2dDestBlend[] =
{
    P2D_ZERO,
    P2D_ONE,
    P2D_ZERO,
    P2D_ONE_MINUS_SRC_ALPHA,
    P2D_ONE,
    P2D_ONE_MINUS_SRC_ALPHA,
    P2D_DST_ALPHA
};

static const unsigned p2dSrcBlend[] =
{
    P2D_ONE,
    P2D_ONE,
    P2D_DST_COLOR,
    P2D_SRC_ALPHA,
    P2D_SRC_ALPHA,
    P2D_ONE,
    P2D_ONE_MINUS_DST_ALPHA
};

ParticleEmitter2D::ParticleEmitter2D(Context* context) : Sprite2D(context),
    duration_(0.0f),
    maxParticles_(0),
    minParticleLifespan_(0.0f),
    maxParticleLifespan_(0.0f),
    minStartSize_(0.0f),
    maxStartSize_(0.0f),
    minEndSize_(0.0f),
    maxEndSize_(0.0f),
    minAngle_(0.0f),
    maxAngle_(0.0f),
    minStartSpin_(0.0f),
    maxStartSpin_(0.0f),
    minEndSpin_(0.0f),
    maxEndSpin_(0.0f),
    emitterType_(ET_GRAVITY),
    minSpeed_(0.0f),
    maxSpeed_(0.0f),
    gravity_(0.0f, 0.0f),
    minRadialAcceleration_(0.0f),
    maxRadialAcceleration_(0.0f),
    minTangentialAcceleration_(0.0f),
    maxTangentialAcceleration_(0.0f),
    minStartRadius_(0.0f),
    maxStartRadius_(0.0f),
    minEndRadius_(0.0f),
    maxEndRadius_(0.0f),
    minRotatePerSecond_(0.0f),
    maxRotatePerSecond_(0.0f),
    minPosition_(0.0f, 0.0f),
    maxPosition_(0.0f, 0.0f),
    timeToLive_(0.0f),
    emissionRate_(0.0f)
{
    SetUseTextureSize(false);
    SetUseWholeTexture(true);
}

ParticleEmitter2D::~ParticleEmitter2D()
{
}

void ParticleEmitter2D::RegisterObject(Context* context)
{
    context->RegisterFactory<ParticleEmitter2D>(GEOMETRY_CATEGORY);

    COPY_BASE_ATTRIBUTES(ParticleEmitter2D, Sprite2D);
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

    Vector3 worldPosition = GetNode()->GetWorldPosition();
    Vector2 currPosition(worldPosition.x_, worldPosition.y_);

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
                if (radial.x_ || radial.y_)
                    radial.Normalize();

                Vector2 tangential(-radial.y_, radial.x_);

                radial *= particle.radialAccel_;
                tangential *= particle.tangentialAccel_;

                Vector2 accelaration = gravity_ + radial + tangential;
                accelaration *= timeStep;
                
                // Speed.
                particle.velocityX_ += accelaration.x_;
                particle.velocityY_ += accelaration.y_;

                // Position.
                particle.position_.x_ += particle.velocityX_ * timeStep;
                particle.position_.y_ += particle.velocityY_ * timeStep;
            }
            else
            {	
                particle.angle_ += particle.degreesPerSecond_ * timeStep;
                particle.radius_ += particle.radiusDelta_ * timeStep;

                particle.position_.x_ = -Cos(particle.angle_) * particle.radius_;
                particle.position_.y_ = -Sin(particle.angle_) * particle.radius_;
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

bool ParticleEmitter2D::Load(const String& fileName)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    PropertyList* plist = cache->GetResource<PropertyList>(fileName);
    if (!plist)
        return false;

    const PLDictionary& root = plist->GetRoot();

    int maxParticles = (int)root.GetFloat("maxParticles");
    SetMaxParticles(maxParticles);

    minAngle_ = root.GetFloat("angle");
    maxAngle_ = minAngle_ + root.GetFloat("angleVariance");

    duration_ = root.GetFloat("duration");

    // Blend mode
    int blendFuncSource = root.GetInt("blendFuncSource");
    int blendFuncDestination = root.GetInt("blendFuncDestination");
    blendMode_ = BLEND_REPLACE;
    for (int i = 0; i < MAX_BLENDMODES; ++i)
    {
        if (blendFuncSource == p2dSrcBlend[i] && blendFuncDestination == p2dDestBlend[i])
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
    minStartColor_ = Color(r - rv, g - gv, b - bv, a - av);
    maxStartColor_ = Color(r + rv, g + gv, b + bv, a + av);

    r = root.GetFloat("finishColorRed");
    g = root.GetFloat("finishColorGreen");
    b = root.GetFloat("finishColorBlue");
    a = root.GetFloat("finishColorAlpha");    
    rv = root.GetFloat("finishColorVarianceRed");
    gv = root.GetFloat("finishColorVarianceGreen");
    bv = root.GetFloat("finishColorVarianceBlue");
    av = root.GetFloat("finishColorVarianceAlpha");
    minEndColor_ = Color(r - rv, g - gv, b - bv, a - av);
    maxEndColor_ = Color(r + rv, g + gv, b + bv, a + av);

    float startParticleSize = root.GetFloat("startParticleSize");
    float startParticleSizeVariance = root.GetFloat("startParticleSizeVariance");
    minStartSize_ = startParticleSize - startParticleSizeVariance;
    maxStartSize_ = startParticleSize + startParticleSizeVariance;

    float endParticleSize = root.GetFloat("finishParticleSize");
    float endParticleSizeVariance = root.GetFloat("finishParticleSizeVariance");
    minEndSize_ = endParticleSize - endParticleSizeVariance;
    maxEndSize_ = endParticleSize + endParticleSizeVariance;

    float posX = root.GetFloat("sourcePositionx");
    float posY = root.GetFloat("sourcePositiony");
    float posVarX = root.GetFloat("sourcePositionVariancex");
    float posVarY = root.GetFloat("sourcePositionVariancey");
    minPosition_ = Vector2(posX - posVarX, posY - posVarY);
    maxPosition_ = Vector2(posX + posVarX, posY + posVarY);

    float rotationStart = root.GetFloat("rotationStart");
    float rotationStartVariance = root.GetFloat("rotationStartVariance");
    minStartSpin_ = rotationStart - rotationStartVariance;
    maxStartSpin_ = rotationStart + rotationStartVariance;

    float rotationEnd = root.GetFloat("rotationEnd");
    float rotationEndVariance = root.GetFloat("rotationEndVariance");
    minEndSpin_ = rotationEnd + rotationEndVariance;
    maxEndSpin_ = rotationEnd + rotationEndVariance;

    emitterType_ = (EmitterType2D)(int)root.GetFloat("emitterType");
    if (emitterType_ == ET_GRAVITY)
    {
        gravity_.x_ = root.GetFloat("gravityx");
        gravity_.y_ = root.GetFloat("gravityy");

        float speed = root.GetFloat("speed");
        float speedVariance = root.GetFloat("speedVariance");
        minSpeed_ = speed - speedVariance;
        maxSpeed_ = speed + speedVariance;

        float radialAcceleration = root.GetFloat("radialAcceleration");
        float radialAccelVariance = root.GetFloat("radialAccelVariance");
        minRadialAcceleration_ = radialAcceleration - radialAccelVariance;
        maxRadialAcceleration_ = radialAcceleration + radialAccelVariance;

        float tangentialAcceleration = root.GetFloat("tangentialAcceleration");
        float tangentialAccelVariance = root.GetFloat("tangentialAccelVariance");
        minTangentialAcceleration_ = tangentialAcceleration - tangentialAccelVariance;
        maxTangentialAcceleration_ = tangentialAcceleration + tangentialAccelVariance;
    }
    else
    {
        minStartRadius_ = root.GetFloat("maxRadius");
        maxStartRadius_ = minStartRadius_ + root.GetFloat("maxRadiusVariance");

        minEndRadius_ = maxEndRadius_ = root.GetFloat("minRadius");

        minRotatePerSecond_ = root.GetFloat("rotatePerSecond");
        maxRotatePerSecond_ = minRotatePerSecond_ + root.GetFloat("rotatePerSecondVariance");
    }

    float particleLifespan = root.GetFloat("particleLifespan");
    float particleLifespanVariance = root.GetFloat("particleLifespanVariance");
    minParticleLifespan_ = particleLifespan - particleLifespanVariance;
    maxParticleLifespan_ = particleLifespan + particleLifespanVariance;

    emissionRate_ = maxParticles / particleLifespan;

    String textureFileName = root.GetString("textureFileName");
    Texture2D* texture = cache->GetResource<Texture2D>(textureFileName);
    if (!texture)
        return false;
    SetTexture(texture);

    return true;
}

void ParticleEmitter2D::SetEmitterType(EmitterType2D type)
{
    emitterType_ = type;
}

void ParticleEmitter2D::SetGravity(const Vector2& gravity)
{
    gravity_ = gravity;
}

void ParticleEmitter2D::SetMaxParticles(int maxParticles)
{
    maxParticles_ = maxParticles;

    particles_.Reserve(maxParticles);
    vertices_.Reserve(maxParticles * 6);
}

void ParticleEmitter2D::SetDuration(float duration)
{
    duration_ = duration;
}

void ParticleEmitter2D::SetMinParticleLifespan(float lifeSpan)
{
    minParticleLifespan_ = lifeSpan;
}

void ParticleEmitter2D::SetMaxParticleLifespan(float lifeSpan)
{
    maxParticleLifespan_ = lifeSpan;
}

void ParticleEmitter2D::SetMinSpeed(float speed)
{
    minSpeed_ = speed;
}

void ParticleEmitter2D::SetMaxSpeed(float speed)
{
    maxSpeed_ = speed;
}

void ParticleEmitter2D::SetMinRadialAcceleration(float accel)
{
    minRadialAcceleration_ = accel;
}

void ParticleEmitter2D::SetMaxRadialAcceleration(float accel)
{
    maxRadialAcceleration_ = accel;
}

void ParticleEmitter2D::SetMinTangentialAcceleration(float accel)
{
    minTangentialAcceleration_ = accel;
}

void ParticleEmitter2D::SetMaxTangentialAcceleration(float accel)
{
    maxTangentialAcceleration_ = accel;
}

void ParticleEmitter2D::SetMinStartRadius(float radius)
{
    minStartRadius_ = radius;
}

void ParticleEmitter2D::SetMaxStartRadius(float radius)
{
    maxStartRadius_ = radius;
}

void ParticleEmitter2D::SetMinEndRadius(float radius)
{
    minEndRadius_ = radius;
}

void ParticleEmitter2D::SetMaxEndRadius(float radiusVar)
{
    maxEndRadius_ = radiusVar;
}

void ParticleEmitter2D::SetMinRotatePerSecond(float rotatePerSecond)
{
    minRotatePerSecond_= rotatePerSecond;
}

void ParticleEmitter2D::SetMaxRotatePerSecond(float rotatePerSecond)
{
    maxRotatePerSecond_= rotatePerSecond;
}

void ParticleEmitter2D::SetMinAngle(float angle)
{
    minAngle_ = angle;
}

void ParticleEmitter2D::SetMaxAngle(float angle)
{
    maxAngle_ = angle;
}

void ParticleEmitter2D::SetMinStartColor(const Color& color)
{
    minStartColor_ = color;
}

void ParticleEmitter2D::SetMaxStartColor(const Color& color)
{
    maxStartColor_ = color;
}


void ParticleEmitter2D::SetMinEndColor(const Color& color)
{
    minEndColor_ = color;
}

void ParticleEmitter2D::SetMaxEndColor(const Color& color)
{
    maxEndColor_ = color;
}

void ParticleEmitter2D::SetMinStartSize(float size)
{
    minStartSize_ = size;
}

void ParticleEmitter2D::SetMaxStartSize(float size)
{
    maxStartSize_= size;
}


void ParticleEmitter2D::SetMinEndSize(float size)
{
    minEndSize_ = size;
}

void ParticleEmitter2D::SetMaxEndSize(float size)
{
    maxEndSize_ = size;
}

void ParticleEmitter2D::SetMinStartSpin(float spin)
{
    minStartSpin_ = spin;
}

void ParticleEmitter2D::SetMaxStartSpin(float spin)
{
    maxStartSpin_ = spin;
}

void ParticleEmitter2D::SetMinEndSpin(float spin)
{
    minEndSpin_ = spin;
}

void ParticleEmitter2D::SetMaxEndSpin(float spin)
{
    maxEndSpin_ = spin;
}

void ParticleEmitter2D::UpdateVertices()
{
    if (!verticesDirty_)
        return;

    vertices_.Clear();

    if (!texture_)
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

    float leftU = 0.0f;
    float bottomV = 0.0f;
    float rightU = 1.0f;
    float topV = 1.0f;
    if (!useWholeTexture_)
    {
        float invTexW = 1.0f;
        float invTexH = 1.0f;
        if (texture_)
        {
            invTexW = 1.0f / (float)texture_->GetWidth();
            invTexH = 1.0f / (float)texture_->GetHeight();
        }

        leftU = textureRect_.left_ * invTexW;
        rightU = textureRect_.right_ * invTexW;
        bottomV = textureRect_.bottom_ * invTexH;
        topV = textureRect_.top_ * invTexH;
    }

    vertex0.uv_ = Vector2(leftU, bottomV);
    vertex1.uv_ = Vector2(leftU, topV);
    vertex2.uv_ = Vector2(rightU, topV);
    vertex3.uv_ = Vector2(rightU, bottomV);

    for (int i = 0; i < particles_.Size(); ++i)
    {
        Particle2D& p = particles_[i];

        float width = p.size_;
        float height = p.size_;

        float leftX = -width * 0.5f;
        float bottomY = -height * 0.5f;

        vertex0.position_ = Vector3(p.position_.x_ + leftX, p.position_.y_ + bottomY, 0.0f);
        vertex1.position_ = Vector3(p.position_.x_ + leftX, p.position_.y_ + bottomY + height, 0.0f);
        vertex2.position_ = Vector3(p.position_.x_ + leftX + width, p.position_.y_ + bottomY + height, 0.0f);
        vertex3.position_ = Vector3(p.position_.x_ + leftX + width, p.position_.y_ + bottomY, 0.0f);

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

    particle.timeToLive_ = Lerp(minParticleLifespan_, maxParticleLifespan_, Random(1.0f));
    particle.timeToLive_ = Max(particle.timeToLive_, 0.01f);
    float invTimeToLive = 1.0f / particle.timeToLive_;

    particle.position_.x_ = Lerp(minPosition_.x_, maxPosition_.x_, Random(1.0f));
    particle.position_.y_ = Lerp(minPosition_.y_, maxPosition_.y_, Random(1.0f));

    particle.color_.r_ = Lerp(minStartColor_.r_, maxStartColor_.r_, Random(1.0f));
    particle.color_.g_ = Lerp(minStartColor_.g_, maxStartColor_.g_, Random(1.0f));
    particle.color_.b_ = Lerp(minStartColor_.b_, maxStartColor_.b_, Random(1.0f));
    particle.color_.a_ = Lerp(minStartColor_.a_, maxStartColor_.a_, Random(1.0f));
    particle.color_.Clip(true);

    Color endColor;
    endColor.r_ = Lerp(minEndColor_.r_, maxEndColor_.r_, Random(1.0f));
    endColor.g_ = Lerp(minEndColor_.g_, maxEndColor_.g_, Random(1.0f));
    endColor.b_ = Lerp(minEndColor_.b_, maxEndColor_.b_, Random(1.0f));
    endColor.a_ = Lerp(minEndColor_.a_, maxEndColor_.a_, Random(1.0f));
    endColor.Clip(true);

    particle.colorDelta_.r_ = (endColor.r_ - particle.color_.r_) * invTimeToLive;
    particle.colorDelta_.g_ = (endColor.g_ - particle.color_.g_) * invTimeToLive;
    particle.colorDelta_.b_ = (endColor.b_ - particle.color_.b_) * invTimeToLive;
    particle.colorDelta_.a_ = (endColor.a_ - particle.color_.a_) * invTimeToLive;

    particle.size_ = Lerp(minStartSize_, maxStartSize_, Random(1.0f));
    particle.size_ = Max(particle.size_, 0.0f);
    float endSize = Lerp(minEndSize_, maxEndSize_, Random(1.0f));
    endSize = Max(endSize, 0.0f);
    particle.sizeDelta_ = (endSize - particle.size_) * invTimeToLive;

    particle.rotation_ = Lerp(minStartSpin_, maxStartSize_, Random(1.0f));
    float endRotation = Lerp(minEndSpin_, maxEndSpin_, Random(1.0f));
    particle.rotationDelta_ = (endRotation - particle.rotation_) * invTimeToLive;

    // ?
    Vector3 worldPosition = GetNode()->GetWorldPosition();
    particle.startPos_ = Vector2(worldPosition.x_, worldPosition.y_);

    float angle = Lerp(minAngle_, maxAngle_, Random(1.0f));

    if (emitterType_ == ET_GRAVITY)
    {
        Vector2 dir(Cos(angle), Sin(angle));
        float speed = Lerp(minSpeed_, maxSpeed_, Random(1.0f));
        particle.velocityX_ = dir.x_ * speed;
        particle.velocityY_ = dir.y_ * speed;
        particle.radialAccel_ = Lerp(minRadialAcceleration_, maxRadialAcceleration_, Random(1.0f));
        particle.tangentialAccel_ = Lerp(minTangentialAcceleration_, maxTangentialAcceleration_, Random(1.0f));
    }
    else
    {
        particle.radius_ = Lerp(minStartRadius_, maxStartRadius_, Random(1.0f));
        float endRadius = Lerp(minEndRadius_, maxEndRadius_, Random(1.0f));
        particle.radiusDelta_ = (endRadius - particle.radius_) * invTimeToLive;
        particle.angle_ = angle;
        particle.degreesPerSecond_ = Lerp(minRotatePerSecond_, maxRotatePerSecond_, Random(1.0f));
    }
}

void ParticleEmitter2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // Store scene's timestep and use it instead of global timestep, as time scale may be other than 1
    using namespace ScenePostUpdate;

    // lastTimeStep_ = eventData[P_TIMESTEP].GetFloat();

    // If no invisible update, check that the billboardset is in view (framenumber has changed)
    //if (updateInvisible_ || viewFrameNumber_ != lastUpdateFrameNumber_)
    //{
    //    lastUpdateFrameNumber_ = viewFrameNumber_;
    //    needUpdate_ = true;
    //    MarkForUpdate();
    //}

    MarkForUpdate();
}

void ParticleEmitter2D::OnSetEnabled()
{
    Sprite2D::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
        else
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
    }
}

void ParticleEmitter2D::OnNodeSet(Node* node)
{
    Sprite2D::OnNodeSet(node);

    if (node)
    {
        Scene* scene = GetScene();
        if (scene && IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
    }
}

}