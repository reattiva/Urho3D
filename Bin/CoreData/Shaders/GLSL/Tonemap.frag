#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

uniform float cTonemapExposer;
uniform float cTonemapWhiteCutoff;

varying vec2 vTexCoord;
varying vec2 vScreenPos;

void main()
{
    #ifdef REINHARD
    gl_FragColor = vec4(ReinhardTonemap(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposer), 1.0);
    #endif

    #ifdef FILMIC
    vec3 rgb = FilmicTonemap(texture2D(sDiffMap, vScreenPos).rgb * cTonemapExposer) / 
        FilmicTonemap(vec3(cTonemapWhiteCutoff, cTonemapWhiteCutoff, cTonemapWhiteCutoff));
    gl_FragColor = vec4(rgb, 1.0);
    #endif
}
