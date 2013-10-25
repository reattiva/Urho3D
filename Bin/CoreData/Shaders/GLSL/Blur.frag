#include "Uniforms.frag"
#include "Samplers.frag"
#include "PostProcess.frag"

uniform float cBlurSigma;
uniform float cBlurScale;
uniform vec2 cBlurDir;
uniform vec2 cBlurInvSize;

varying vec2 vTexCoord;
varying vec2 vScreenPos;

void main()
{
    #ifdef BLUR5
        gl_FragColor = GaussianBlur(5, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR7
        gl_FragColor = GaussianBlur(7, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR9
        gl_FragColor = GaussianBlur(9, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR17
        gl_FragColor = GaussianBlur(17, cBlurSigma, cBlurDir, cBlurInvSize * cBlurScale, sDiffMap, vTexCoord);
    #endif
}
