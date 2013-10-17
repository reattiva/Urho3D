#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

uniform float cBloomHDRThreshold;
uniform float cBloomHDRBlurSigma;
uniform float cBloomHDRBlurScale;
uniform vec2 cBloomHDRBlurDir;
uniform vec2 cBloomHDRMix;
uniform vec2 cBright2InvSize;
uniform vec2 cBright4InvSize;
uniform vec2 cBright8InvSize;
uniform vec2 cBright16InvSize;

varying vec2 vTexCoord;
varying vec2 vScreenPos;

void main()
{
    #ifdef BRIGHT
    vec3 rgb = texture2D(sDiffMap, vScreenPos).rgb;
    vec3 rgbt = vec3(cBloomHDRThreshold, cBloomHDRThreshold, cBloomHDRThreshold);
    gl_FragColor = vec4(max(rgb - rgbt, 0.0), 1.0) / (1.0 - cBloomHDRThreshold);
    #endif

    #ifdef BLUR2
    gl_FragColor = GaussianBlur(5, cBloomHDRBlurSigma, cBloomHDRBlurDir, cBright2InvSize * cBloomHDRBlurScale, sDiffMap, vScreenPos);
    #endif

    #ifdef BLUR4
    gl_FragColor = GaussianBlur(5, cBloomHDRBlurSigma, cBloomHDRBlurDir, cBright4InvSize * cBloomHDRBlurScale, sDiffMap, vScreenPos);
    #endif

    #ifdef BLUR8
    gl_FragColor = GaussianBlur(5, cBloomHDRBlurSigma, cBloomHDRBlurDir, cBright8InvSize * cBloomHDRBlurScale, sDiffMap, vScreenPos);
    #endif

    #ifdef BLUR16
    gl_FragColor = GaussianBlur(5, cBloomHDRBlurSigma, cBloomHDRBlurDir, cBright16InvSize * cBloomHDRBlurScale, sDiffMap, vScreenPos);
    #endif

    #ifdef COMBINE2
    gl_FragColor = vec4(CombineColors(texture2D(sDiffMap, vScreenPos).rgb, texture2D(sNormalMap, vTexCoord).rgb, cBloomHDRMix), 1.0);
    #endif

    #ifdef COMBINE4
    gl_FragColor = vec4(CombineColors(texture2D(sDiffMap, vScreenPos).rgb, texture2D(sNormalMap, vTexCoord).rgb, cBloomHDRMix), 1.0);
    #endif

    #ifdef COMBINE8
    gl_FragColor = vec4(CombineColors(texture2D(sDiffMap, vScreenPos).rgb, texture2D(sNormalMap, vTexCoord).rgb, cBloomHDRMix), 1.0);
    #endif

    #ifdef COMBINE16
    gl_FragColor = vec4(CombineColors(texture2D(sDiffMap, vScreenPos).rgb, texture2D(sNormalMap, vTexCoord).rgb, cBloomHDRMix), 1.0);
    #endif
}
