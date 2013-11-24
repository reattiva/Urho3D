#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

uniform float cTonemapExposureBias;
uniform float cTonemapMaxWhite;

varying vec2 vScreenPos;

void main()
{
    #ifdef REINHARDEQ3
    vec3 color = max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0);
    color = ReinhardEq3Tonemap(color);
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef REINHARDEQ4
    vec3 color = max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0);
    color = ReinhardEq4Tonemap(color, cTonemapMaxWhite);
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef UNCHARTED2
    vec3 color = max(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0);
    color = Uncharted2Tonemap(color) / Uncharted2Tonemap(vec3(cTonemapMaxWhite));
    gl_FragColor = vec4(color, 1.0);
    #endif
}
