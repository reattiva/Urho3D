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

#pragma once

#include "Drawable2D.h"

namespace Urho3D
{

/// 2D emitter type.
enum EmitterType2D
{
    /// Gravity.
    ET_GRAVITY = 0,
    /// Radius.
    ET_RADIUS,
};

/// 2D particle.
struct URHO3D_API Particle2D
{
    /// Position.
    Vector2 position_;
    /// Color.
    Color color_;
    /// Color delta.
    Color colorDelta_;
    /// Size.
    float size_;
    /// Size delta.
    float sizeDelta_;
    /// Rotation.
    float rotation_;
    /// Rotation delta.
    float rotationDelta_;
    /// Lifetime.
    float timeToLive_;
    union
    {
        /// Velocity X in gravity mode.
        float velocityX_;
        /// Angle in radius mode.
        float angle_;
    };
    union
    {
        /// Velocity Y in gravity mode.
        float velocityY_;
        /// Degrees per second in radius mode.
        float degreesPerSecond_;
    };
    union
    {
        /// Radial accel in gravity mode.
        float radialAccel_;
        /// Radius in radius mode.
        float radius_;
    };
    union
    {
        /// Tangential accel in gravity mode.
        float tangentialAccel_;
        /// Radius delta in radius mode.
        float radiusDelta_;
    };
};

/// 2D sprite component.
class URHO3D_API ParticleEmitter2D : public Drawable2D
{
    OBJECT(ParticleEmitter2D);

public:
    /// Construct.
    ParticleEmitter2D(Context* context);
    /// Destruct.
    ~ParticleEmitter2D();
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();
    /// Update before octree reinsertion. Is called from a worker thread.
    virtual void Update(const FrameInfo& frame);
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame);
    
    /// Load from file.
    bool Load(const String& fileName);

    /// Set duration.
    void SetDuration(float duration);
    /// Set max particles.
    void SetMaxParticles(unsigned maxParticles);
    /// Set min particle life span.
    void SetMinParticleLifespan(float lifeSpan);
    /// Set max particle life span.
    void SetMaxParticleLifespan(float lifeSpan);
    /// Set min start size.
    void SetMinStartSize(float size);
    /// Set min start size.
    void SetMaxStartSize(float size);
    /// Set min end size.
    void SetMinEndSize(float size);
    /// Set max end size.
    void SetMaxEndSize(float size);
    /// Set min angle.
    void SetMinAngle(float angle);
    /// Set max angle.
    void SetMaxAngle(float angle);
    /// Set min start spin.
    void SetMinStartSpin(float spin);
    /// Set max start spin.
    void SetMaxStartSpin(float spin);
    /// Set min end spin.
    void SetMinEndSpin(float spin);
    /// Set max end spin.
    void SetMaxEndSpin(float spin);
    /// Set min start color.
    void SetMinStartColor(const Color& color);
    /// Set max start color.
    void SetMaxStartColor(const Color& color);
    /// Set min end color.
    void SetMinEndColor(const Color& color);
    /// Set max end color.
    void SetMaxEndColor(const Color& color);
    /// Set emitter type.
    void SetEmitterType(EmitterType2D type);
    /// Set min speed.
    void SetMinSpeed(float speed);
    /// Set max speed.
    void SetMaxSpeed(float speed);
    /// Set gravity.
    void SetGravity(const Vector2& gravity);
    /// Set min radial acceleration.
    void SetMinRadialAcceleration(float accel);
    /// Set max radial acceleration.
    void SetMaxRadialAcceleration(float accel);    
    /// Set min tangential acceleration.
    void SetMinTangentialAcceleration(float accel);
    /// Set max tangential acceleration.
    void SetMaxTangentialAcceleration(float accel);
    /// Set min start radius.
    void SetMinStartRadius(float radius);
    /// Set max start radius
    void SetMaxStartRadius(float radius);
    /// Set min end radius.
    void SetMinEndRadius(float radius);
    /// Set max end radius.
    void SetMaxEndRadius(float radius);
    /// Set min rotate per second.
    void SetMinRotatePerSecond(float rotatePerSecond);
    /// Set max rotate per second.
    void SetMaxRotatePerSecond(float rotatePerSecond);
    /// Set min position.
    void SetMinPosition(const Vector2& position);
    /// Set max position.
    void SetMaxPosition(const Vector2& position);
    /// Return duration.
    float GetDuration() const { return duration_; }
    /// Return max particles.
    unsigned GetMaxParticles() const { return maxParticles_; }
    /// Return min particle life time.
    float GetMinParticleLifespan() const { return particleLifespanMin_; }
    /// Return max particle life time.
    float GetMaxParticleLifespan() const { return particleLifespanMax_; }
    /// Return min start size.
    float GetMinStartSize() const { return startSizeMin_; }
    /// Return max start size.
    float GetMaxStartSize() const { return startSizeMax_; }
    /// Return min end size.
    float GetMinEndSize() const { return endSizeMin_; }
    /// Return max end size.
    float GetMaxEndSize() const { return endSizeMax_; }
    /// Return min angle.
    float GetMinAngle() const { return angleMin_; }
    /// Return max angle.
    float GetMaxAngle() const { return angleMax_; }
    /// Return min start spin.
    float GetMinStartSpin() const { return startSpinMin_; }
    /// Return max start spin.
    float GetMaxStartSpin() const { return startSpinMax_; }
    /// Return min end spin.
    float GetMinEndSpin() const { return  endSpinMin_; }
    /// Return max end spin.
    float GetMaxEndSpin() const { return endSpinMax_; }
    /// Return min start color.
    const Color& GetMinStartColor() const { return startColorMin_; }
    /// Return max start color.
    const Color& GetMaxStartColor() const { return startColorMax_; }
    /// Return min end color.
    const Color& GetMinEndColor() const { return endColorMin_; }
    /// Return max end color.
    const Color& GetMaxEndColor() const { return endColorMax_; }
    /// Return emitter type.
    EmitterType2D GetEmitterType() const { return emitterType_; }    
    /// Return min speed.
    float GetMinSpeed() const { return speedMin_; }
    /// Return max speed.
    float GetMaxSpeed() const { return speedMax_; }
    /// Return gravity.
    const Vector2& GetGravity() const { return gravity_; }
    /// Return min radial acceleration.
    float GetMinRadialAcceleration() const { return radialAccelerationMin_; }
    /// Return max radial acceleration.
    float GetMaxRadialAcceleration() const { return radialAccelerationMax_; }
    /// Return min tangential acceleration.
    float GetMinTangentialAcceleration() const { return tangentialAccelerationMin_; }
    /// Return max tangential acceleration.
    float GetMaxTangentialAcceleration() const { return tangentialAccelerationMax_; }
    /// Return min start radius.
    float GetMinStartRadius() const { return startRadiusMin_; }
    /// Return max start radius.
    float GetMaxStartRadius() const { return startRadiusMax_; }
    /// Return min end radius.
    float GetMinEndRadius() const { return endRadiusMin_; }
    /// Return max end radius.
    float GetMaxEndRadius() const { return endRadiusMax_; }
    /// Return min rotate per second.
    float GetMinRotatePerSecond() const { return rotatePerSecondMin_; }
    /// Return max rotate per second.
    float GetMaxRotatePerSecond() const { return rotatePerSecondMax_; }
    /// Return min position.
    const Vector2& GetMinPosition() const { return positionMin_; }
    /// Return max position.
    const Vector2& GetMaxPosition() const { return positionMax_; }

private:
    /// Handle node being assigned.
    virtual void OnNodeSet(Node* node);
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();   
    /// Update vertices.
    virtual void UpdateVertices();
    /// Emit new particle.
    void EmitNewParticle();
    /// Handle scene post-update event.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Duration.
    float duration_;
    /// Max particles.
    unsigned maxParticles_;
    /// Particle lifetime minimum.
    float particleLifespanMin_;
    /// Particle lifetime maximum.
    float particleLifespanMax_;
    /// Start size minimum.
    float startSizeMin_;
    /// Start size maximum.
    float startSizeMax_;
    /// End size minimum.
    float endSizeMin_;
    /// End size maximum.
    float endSizeMax_;
    /// Angle minimum.
    float angleMin_;
    /// Angle maximum.
    float angleMax_;
    /// Start spin minimum.
    float startSpinMin_;
    /// Start spin maximum.
    float startSpinMax_;
    /// End spin minimum.
    float endSpinMin_;
    /// End spin maximum.
    float endSpinMax_;
    /// Start color minimum.
    Color startColorMin_;
    /// Start color maximum.
    Color startColorMax_;
    /// End color minimum.
    Color endColorMin_;
    /// End color maximum.
    Color endColorMax_;
    /// Emitter type.
    EmitterType2D emitterType_;
    /// Speed minimum.
    float speedMin_;
    /// Speed maximum.
    float speedMax_;
    /// Gravity
    Vector2 gravity_;
    /// Radial acceleration minimum.
    float radialAccelerationMin_;
    /// Radial acceleration maximum.
    float radialAccelerationMax_;
    /// Tangential acceleration minimum.
    float tangentialAccelerationMin_;
    /// Tangential acceleration maximum.
    float tangentialAccelerationMax_;
    /// Start radius minimum.
    float startRadiusMin_;
    /// Start radius maximum.
    float startRadiusMax_;
    /// End radius minimum.
    float endRadiusMin_;
    /// End radius maximum.
    float endRadiusMax_;
    /// Rotate per second minimum.
    float rotatePerSecondMin_;
    /// Rotate per second maximum.
    float rotatePerSecondMax_;
    /// Position minimum.
    Vector2 positionMin_;
    /// Position maximum.
    Vector2 positionMax_;
    /// Lifetime.
    float timeToLive_;
    /// Emission rate.
    float emissionRate_;
    /// Particles.
    Vector<Particle2D> particles_;
};

}

