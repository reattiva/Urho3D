#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

varying vec2 vTexCoord;
varying vec2 vScreenPos;

void main()
{
    vec3 original = texture2D(sDiffMap, vScreenPos).rgb;
    gl_FragColor = vec4(ColorCorrection(original, sVolumeMap), 1.0);
}
