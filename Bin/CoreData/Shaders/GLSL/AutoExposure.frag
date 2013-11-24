#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

uniform vec2 cAutoExposureLumMinMax;
uniform float cAutoExposureLumAdaptRate;
uniform float cAutoExposureMiddleGrey;
uniform float cAutoExposureMaxWhite;
uniform vec2 cHDR128InvSize;
uniform vec2 cLum64InvSize;
uniform vec2 cLum16InvSize;
uniform vec2 cLum4InvSize;

varying vec2 vTexCoord;
varying vec2 vScreenPos;

float GatherAvgLum(sampler2D texSampler, vec2 texCoord, vec2 texelSize)
{
    float lumAvg = 0.0;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, -1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(-1.0, 1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, 1.0) * texelSize).r;
    lumAvg += texture2D(texSampler, texCoord + vec2(1.0, -1.0) * texelSize).r;
    return lumAvg / 4.0;
}

void main()
{
    #ifdef LUMINANCE64
    vec3 color = vec3(0.0);
    color += texture2D(sDiffMap, vTexCoord + vec2(-1.0, -1.0) * cHDR128InvSize).rgb;
    color += texture2D(sDiffMap, vTexCoord + vec2(-1.0, 1.0) * cHDR128InvSize).rgb;
    color += texture2D(sDiffMap, vTexCoord + vec2(1.0, 1.0) * cHDR128InvSize).rgb;
    color += texture2D(sDiffMap, vTexCoord + vec2(1.0, -1.0) * cHDR128InvSize).rgb;
    color /= 4.0;
    gl_FragColor.r = 1e-5 + log(dot(color, LumWeights));
    #endif

    #ifdef LUMINANCE16
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum64InvSize);
    #endif

    #ifdef LUMINANCE4
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum16InvSize);
    #endif

    #ifdef LUMINANCE1
    float lum = GatherAvgLum(sDiffMap, vTexCoord, cLum4InvSize);
    gl_FragColor.r = exp(lum);
    #endif

    #ifdef ADAPTLUMINANCE
    float adaptedLum = texture2D(sDiffMap, vTexCoord).r;
    float lum = texture2D(sNormalMap, vTexCoord).r;
    lum = clamp(lum, cAutoExposureLumMinMax.x, cAutoExposureLumMinMax.y);
    gl_FragColor.r = adaptedLum + (lum - adaptedLum) * (1.0 - exp(-cDeltaTimePS * cAutoExposureLumAdaptRate));
    #endif

    #ifdef APPLYLUMINANCE
    vec3 color = texture2D(sDiffMap, vScreenPos).rgb;
    float adaptedLum = texture2D(sNormalMap, vTexCoord).r;
    gl_FragColor = vec4(AdjustColorLum(color, adaptedLum, cAutoExposureMiddleGrey, cAutoExposureMaxWhite), 1.0);
    #endif
}
