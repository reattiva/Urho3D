
static const float pi = 3.14159265;

float2 Noise(float2 coord)
{
    float noiseX = clamp(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0);
    float noiseY = clamp(frac(sin(dot(coord, float2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0);
    return float2(noiseX, noiseY) * 2.0 - 1.0;
}

// Adapted: http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
float4 GaussianBlur(int numSamples, float sigma, float2 blurDir, float2 blurSize, sampler2D texSampler, float2 texCoord)
{
	const int numSamplesPerSide = numSamples / 2;

	// Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
	float3 incrementalGaussian;
	incrementalGaussian.x = 1.0 / (sqrt(2.0 * pi) * sigma);
	incrementalGaussian.y = exp(-0.5 / (sigma * sigma));
	incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

	float4 avgValue = float4(0.0, 0.0, 0.0, 0.0);
	float coefficientSum = 0.0;

	avgValue += tex2D(texSampler, texCoord) * incrementalGaussian.x;
	coefficientSum += incrementalGaussian.x;
	incrementalGaussian.xy *= incrementalGaussian.yz;

	for (float i = 1.0; i <= numSamplesPerSide; i++)
	{
		avgValue += tex2D(texSampler, texCoord - i * blurSize * blurDir) * incrementalGaussian.x;
		avgValue += tex2D(texSampler, texCoord + i * blurSize * blurDir) * incrementalGaussian.x;

		coefficientSum += 2.0 * incrementalGaussian.x;
		incrementalGaussian.xy *= incrementalGaussian.yz;
	}

	return avgValue / coefficientSum;
}

float3 ReinhardTonemap(float3 x)
{
	return x / (x + 1.0);
}

// Filmic tone mapping (Uncharted2) (See http://filmicgames.com)
static const float A = 0.15;
static const float B = 0.50;
static const float C = 0.10;
static const float D = 0.20;
static const float E = 0.02;
static const float F = 0.30;

float3 FilmicTonemap(float3 x)
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float3 CombineColors(float3 color1, float3 color2, float2 colorMix)
{
	float3 rgb1 = color1 * colorMix.x;
    float3 rgb2 = color2 * colorMix.y;
    // Prevent oversaturation
    rgb1 *= saturate(1.0 - rgb2);
    return rgb1 + rgb2;
}
