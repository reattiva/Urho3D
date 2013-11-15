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
#include "Texture.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];
extern const char* GEOMETRY_CATEGORY;

ParticleEmitter2D::ParticleEmitter2D(Context* context) : Sprite2D(context),
    duration_(0.0f),
    maxParticles_(0),
    emissionRate_(0.0f),
    minParticleLifespan_(0.0f),
    maxParticleLifespan_(0.0f),

    emitterType_(ET_GRAVITY),
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
    minAngle_(0.0f),
    maxAngle_(0.0f),
    minSpeed_(0.0f),
    maxSpeed_(0.0f),

    minStartSize_(0.0f),
    maxStartSize_(0.0f),
    minEndSize_(0.0f),
    maxEndSize_(0.0f),
    minStartSpin_(0.0f),
    maxStartSpin_(0.0f),
    minEndSpin_(0.0f),
    maxEndSpin_(0.0f),
    timeToLive_(0.0f)
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

    if (duration_ < 0.0f || timeToLive_ < duration_)     
    {
        unsigned numParticles = (unsigned)(emissionRate_ * timeToLive_);
        while (particles_.Size() < numParticles)
            EmitNewParticle();
    }

    //Vector2 currentPosition = Vector2::ZERO;
    //if (positionType_== PT_FREE)
    //    currentPosition = GetNode()->GetWorldPosition();
    //else
    //{
    //    // currentPosition = m_tPosition;
    //    currentPosition = GetNode()->GetWorldPosition();            
    //}

    unsigned numParticles = particles_.Size();
    for (unsigned i = 0; i < numParticles; ++i)
    {
        Particle2D& particle = particles_[i];

        particle.timeToLive_ -= timeStep;
        if (particle.timeToLive_ > 0.0f)
        {
            if (emitterType_ == ET_GRAVITY)
            {
                Vector2 radital = particle.position_ - particle.startPos_;
                radital.Normalize();

                Vector2 accelaration = Vector2(particle.gravityX_, particle.gravityY_);
                accelaration += radital * particle.radialAccel_;
                accelaration += Vector2(radital.y_, -radital.x_) * particle.tangentialAccel_;

                particle.position_ += accelaration * timeStep;
            }
            else
            {				
                particle.angle_ += particle.angleDelta_ * timeStep;
                particle.radius_ += particle.radiusDelta_* timeStep;

                particle.position_ = Vector2(Cos(particle.angle_), Sin(particle.angle_)) * particle.radius_;
            }

            particle.color_ += particle.colorDelta_ * timeStep;

            particle.size_ += particle.sizeDelta_ * timeStep;
            particle.size_ = Max(particle.size_, 0.0f);

            particle.spin_ += particle.spinDelta_ * timeStep;
        } 
        else 
        {
            --numParticles;
            if (i != numParticles)
                particles_[i--] = particles_[numParticles];
        }
    }
}

bool ParticleEmitter2D::Load(const String& fileName)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    PropertyList* plist = cache->GetResource<PropertyList>(fileName);
    if (!plist)
        return false;

    const PLDictionary& root = plist->GetRoot();

    duration_ = root.GetFloat("duration");
    int maxParticles = root.GetInt("maxParticles");
    SetMaxParticles(maxParticles);
    emissionRate_ = root.GetFloat("emissionRate");

    float particleLifespan = root.GetFloat("particleLifespan");
    float particleLifespanVar = root.GetFloat("particleLifespanVariance");
    minParticleLifespan_ = particleLifespan - particleLifespanVar;
    maxParticleLifespan_ = particleLifespan + particleLifespanVar;

    emitterType_ = (EmitterType2D)root.GetInt("emitterType");
    if (emitterType_ == ET_GRAVITY)
    {
        gravity_.x_ = root.GetFloat("gravityx");
        gravity_.y_ = root.GetFloat("gravityy");

        float radialAccel = root.GetFloat("radialAcceleration");
        float radialAccelVar = root.GetFloat("radialAccelVariance");
        minRadialAcceleration_ = radialAccel - radialAccelVar;
        maxRadialAcceleration_ = radialAccel + radialAccelVar;

        float tangentialAccel= root.GetFloat("tangentialAcceleration");
        float tangentialAccelVar = root.GetFloat("tangentialAccelVariance");
        minTangentialAcceleration_ = tangentialAccel - tangentialAccelVar;
        maxTangentialAcceleration_ = tangentialAccel + tangentialAccelVar;
    }
    else
    {
        float rotatePerSecond = root.GetFloat("rotatePerSecond");
        float rotatePerSecondVar = root.GetFloat("rotatePerSecondVariance");
        minRotatePerSecond_ = rotatePerSecond - rotatePerSecondVar;
        maxRotatePerSecond_ = rotatePerSecond + rotatePerSecondVar;
        
        float startRadius = root.GetFloat("maxRadius");
        float startRadiusVar = root.GetFloat("maxRadiusVariance");
        minStartRadius_ = startRadius - startRadiusVar;
        maxStartRadius_ = startRadius + startRadiusVar;

        minEndRadius_ = maxEndRadius_ = root.GetFloat("minRadius");
    }

    // Blend mode
    int blendFuncSource = root.GetInt("blendFuncSource");
    int blendFuncDestination = root.GetInt("blendFuncDestination");
    if (false)
        blendMode_ = BLEND_ALPHA;
    else
        blendMode_ = BLEND_ADDALPHA;

    String textureFileName = root.GetString("textureFileName");
    Texture* texture = cache->GetResource<Texture>(textureFileName);
    if (!texture)
        return false;
    SetTexture(texture);

    Vector2 position;
    position.x_ = root.GetFloat("sourcePositionx");
    position.y_ = root.GetFloat("sourcePositiony");
    Vector2 positionVar;
    positionVar.x_ = root.GetFloat("sourcePositionVariancex");
    positionVar.y_ = root.GetFloat("sourcePositionVariancey");
    minPosition_ = position - positionVar;
    maxPosition_ = position + positionVar;

    float angle = root.GetFloat("angle");
    float angleVar = root.GetFloat("angleVariance");
    minAngle_ = angle - angleVar;
    maxAngle_ = angle + angleVar;
    float speed = root.GetFloat("speed");
    float speedVar = root.GetFloat("speedVariance");
    minSpeed_ = speed - speedVar;
    maxSpeed_ = speed + speedVar;

    Color startColor;
    startColor.r_ = root.GetFloat("startColorRed");
    startColor.g_ = root.GetFloat("startColorGreen");
    startColor.b_ = root.GetFloat("startColorBlue");
    startColor.a_ = root.GetFloat("startColorAlpha");
    Color startColorVar;
    startColorVar.r_ = root.GetFloat("startColorVarianceRed");
    startColorVar.g_ = root.GetFloat("startColorVarianceGreen");
    startColorVar.b_ = root.GetFloat("startColorVarianceBlue");
    startColorVar.a_ = root.GetFloat("startColorVarianceAlpha");
    minStartColor_ = Color(startColor.r_ - startColorVar.r_, startColor.g_ - startColorVar.g_, startColor.b_ - startColorVar.b_, startColor.a_ - startColorVar.a_);
    maxStartColor_ = Color(startColor.r_ + startColorVar.r_, startColor.g_ + startColorVar.g_, startColor.b_ + startColorVar.b_, startColor.a_ + startColorVar.a_);

    Color endColor;
    endColor.r_ = root.GetFloat("endColorRed");
    endColor.g_ = root.GetFloat("endColorGreen");
    endColor.b_ = root.GetFloat("endColorBlue");
    endColor.a_ = root.GetFloat("endColorAlpha");
    Color endColorVar;
    endColorVar.r_ = root.GetFloat("endColorVarianceRed");    
    endColorVar.g_ = root.GetFloat("endColorVarianceGreen");
    endColorVar.b_ = root.GetFloat("endColorVarianceBlue");
    endColorVar.a_ = root.GetFloat("endColorVarianceAlpha");
    minEndColor_ = Color(endColor.r_ - endColorVar.r_, endColor.g_ - endColorVar.g_, endColor.b_ - endColorVar.b_, endColor.a_ - endColorVar.a_);
    maxEndColor_ = Color(endColor.r_ + endColorVar.r_, endColor.g_ + endColorVar.g_, endColor.b_ + endColorVar.b_, endColor.a_ + endColorVar.a_);

    float startParticleSize = root.GetFloat("startParticleSize");
    float startParticleSizeVar = root.GetFloat("startParticleSizeVariance");
    minStartSize_ = startParticleSize - startParticleSizeVar;
    maxStartSize_ = startParticleSize + startParticleSizeVar;
    
    float endParticleSize = root.GetFloat("endParticleSize");
    float endParticleSizeVar = root.GetFloat("endParticleSizeVariance");
    minEndSize_ = endParticleSize - endParticleSizeVar;
    maxEndSize_ = endParticleSize + endParticleSizeVar;

    float startSpin = root.GetFloat("rotationStart");
    float startSpinVar = root.GetFloat("rotationStartVariance");
    minStartSpin_ = startSpin - startSpinVar;
    maxStartSpin_ = startSpin + startSpinVar;
    
    float endSpin = root.GetFloat("rotationEnd");
    float endSpinVar = root.GetFloat("rotationEndVariance");
    minEndSpin_ = endSpin - endSpinVar;
    maxEndSpin_ = endSpin + endSpinVar;

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
        float width = p.size_* 10;
        float height = p.size_* 10;

        float hotSpotX = flipX_ ? (1.0f - hotSpot_.x_) : hotSpot_.x_;
        float hotSpotY = flipY_ ? (1.0f - hotSpot_.y_) : hotSpot_.y_;
        float leftX = -width * hotSpotX;
        float bottomY = -height * hotSpotY;
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
    Particle2D particle;
    particle.timeToLive_ = Lerp(minParticleLifespan_, maxParticleLifespan_, Random(1.0f));
    float invTimeToLive = 1.0f / particle.timeToLive_;

    Vector3 worldPosition = GetNode()->GetWorldPosition();
    particle.startPos_ = particle.position_ = Vector2(worldPosition.x_, worldPosition.y_);

    if (emitterType_ == ET_GRAVITY)
    {
        particle.gravityX_ = gravity_.x_;
        particle.gravityY_ = gravity_.y_;
        particle.radialAccel_ = Lerp(minRadialAcceleration_, maxRadialAcceleration_, Random(1.0f));
        particle.tangentialAccel_ = Lerp(minTangentialAcceleration_, maxTangentialAcceleration_, Random(1.0f));
    }
    else
    {
        particle.angle_ = Lerp(0.0f, 360.0f, Random(1.0f));
        particle.angleDelta_ = Lerp(minRotatePerSecond_, maxRotatePerSecond_, Random(1.0f));
        particle.radius_ = Lerp(minStartRadius_, maxStartRadius_, Random(1.0f));
        particle.radiusDelta_ = Lerp(minEndRadius_, maxEndRadius_, Random(1.0f));
        particle.radiusDelta_ = (particle.radiusDelta_ - particle.radius_) * invTimeToLive;
    }

    particle.color_ = minStartColor_.Lerp(maxStartColor_, Random(1.0f));
    Color endColor = minEndColor_.Lerp(maxEndColor_, Random(1.0f));
    particle.colorDelta_ = Color(endColor.r_ - particle.color_.r_, endColor.g_ - particle.color_.g_, endColor.b_ - particle.color_.b_, endColor.a_ - particle.color_.a_) * invTimeToLive;

    particle.size_ = Lerp(minStartSize_, maxStartSize_, Random(1.0f));
    particle.sizeDelta_ = Lerp(minEndSize_, maxEndSize_, Random(1.0f));
    particle.sizeDelta_ = (particle.sizeDelta_ - particle.size_) * invTimeToLive;

    particle.spin_ = Lerp(minStartSpin_, maxStartSpin_, Random(1.0f));
    particle.spinDelta_ = Lerp(minEndSpin_, maxEndSpin_, Random(1.0f));
    particle.spinDelta_ = (particle.spinDelta_ - particle.spin_) * invTimeToLive;

    particles_.Push(particle);
}

}