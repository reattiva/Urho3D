
const float pi = 3.14159265;

vec2 Noise(vec2 coord)
{
    float noiseX = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0);
    float noiseY = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0);
    return vec2(noiseX, noiseY) * 2.0 - 1.0;
}

// Adapted: http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
vec4 GaussianBlur(int numSamples, float sigma, vec2 blurDir, vec2 blurSize, sampler2D texSampler, vec2 texCoord)
{
    const int numSamplesPerSide = numSamples / 2;

    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    vec3 incrementalGaussian;
    incrementalGaussian.x = 1.0 / (sqrt(2.0 * pi) * sigma);
    incrementalGaussian.y = exp(-0.5 / (sigma * sigma));
    incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

    vec4 avgValue = vec4(0.0, 0.0, 0.0, 0.0);
    float coefficientSum = 0.0;

    avgValue += texture2D(texSampler, texCoord) * incrementalGaussian.x;
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for (float i = 1.0; i <= numSamplesPerSide; i++)
    {
        avgValue += texture2D(texSampler, texCoord - i * blurSize * blurDir) * incrementalGaussian.x;
        avgValue += texture2D(texSampler, texCoord + i * blurSize * blurDir) * incrementalGaussian.x;

        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    return avgValue / coefficientSum;
}

vec3 ReinhardTonemap(vec3 x)
{
    return x / (x + 1.0);
}

// Filmic tone mapping (Uncharted2) (See http://filmicgames.com)
const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;

vec3 FilmicTonemap(vec3 x)
{
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 CombineColors(vec3 color1, vec3 color2, vec2 colorMix)
{
    vec3 rgb1 = color1 * colorMix.x;
    vec3 rgb2 = color2 * colorMix.y;
    // Prevent oversaturation
    rgb1 *= max(vec3(1.0) - rgb2, vec3(0.0));
    return rgb1 + rgb2;
}

vec3 ColorCorrection(vec3 color, sampler3D lut)
{
    float lutSize = 16.0;
    float scale = (lutSize - 1.0) / lutSize;
    float offset = 1.0 / (2.0 * lutSize);
    return texture3D(lut, clamp(color, 0.0, 1.0) * scale + offset).rgb;
}
