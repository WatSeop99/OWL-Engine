#include <stdlib.h>
#include "../Common.h"
#include "PerlinNoiseGenerator.h"

//https://en.wikipedia.org/wiki/Perlin_noise

// Compute Perlin noise at coordinates x, y.
//float GetValueUsingPerlinNoise(float x, float y)
//{
//	// Determine grid cell coordinates.
//	int x0 = (int)floor(x);
//	int x1 = x0 + 1;
//	int y0 = (int)floor(y);
//	int y1 = y0 + 1;
//
//	// Determine interpolation weights.
//	// Could also use higher order polynomial/s-curve here.
//	float sx = x - (float)x0;
//	float sy = y - (float)y0;
//
//	// Interpolate between grid point gradients.
//	float n0, n1, ix0, ix1, value;
//
//	n0 = DotGridGradient(x0, y0, x, y);
//	n1 = DotGridGradient(x1, y0, x, y);
//	ix0 = Interpolate(n0, n1, sx);
//
//	n0 = DotGridGradient(x0, y1, x, y);
//	n1 = DotGridGradient(x1, y1, x, y);
//	ix1 = Interpolate(n0, n1, sx);
//
//	value = Interpolate(ix0, ix1, sy);
//	return value; // Will return in range -1 to 1. To make it in range 0 to 1, multiply by 0.5 and add 0.5.
//}
//
//// Create pseudorandom direction vector.
//DirectX::SimpleMath::Vector2 RandomGradient(int ix, int iy)
//{
//	// No precomputed gradients mean this works for any number of grid coordinates.
//	const UINT W = 8 * sizeof(UINT);
//	const UINT S = W / 2;
//
//	UINT a = ix;
//	UINT b = iy;
//	a *= 3284157443;
//	b ^= a << S | a >> W - S;
//	b *= 1911520717;
//	a ^= b << S | b >> W - S;
//	a *= 2048419325;
//
//	// float random = a * (DirectX::XM_PI / ~(~0u >> 1));
//
//	float random = (float)rand() / (float)RAND_MAX;
//	DirectX::SimpleMath::Vector2 v;
//	v.x = cos(random);
//	v.y = sin(random);
//
//	return v;
//}
//
//float DotGridGradient(int ix, int iy, float x, float y)
//{
//	// Get gradient from integer coordinates.
//	DirectX::SimpleMath::Vector2 gradient = RandomGradient(ix, iy);
//
//	// Compute the distance vector.
//	float dx = x - (float)ix;
//	float dy = y - (float)iy;
//
//	// Compute the dot-product.
//	return (dx * gradient.x + dy * gradient.y);
//}
//
//float Interpolate(float a0, float a1, float w)
//{
//	/* // You may want clamping by inserting:
//	 * if (0.0 > w) return a0;
//	 * if (1.0 < w) return a1;
//	 */
//
//	// linaer
//	//return (a1 - a0) * w + a0;
//
//	// cubic
//	return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
//
//	// smootherstep
//	return (a1 - a0) * ((w * (w * 6.0f - 15.0f) + 10.0f) * w * w * w) + a0;
//}

float GetNoise()
{
	float random = (float)rand() / (float)RAND_MAX;
	return random * 2.0f - 1.0f;
}

float Interpolate(float a, float b, float blend)
{
	float theta = blend * DirectX::XM_PI;
	float f = (float)(1.0f - cos(theta)) * 0.5f;
	return a * (1.0f - f) + b * f;
}

float GetSmoothNoise(float x, float y)
{
	float corners = (GetNoise() + GetNoise() + GetNoise() + GetNoise()) / 16.0f;
	float sides = (GetNoise() + GetNoise() + GetNoise() + GetNoise()) / 8.0f;
	float center = GetNoise() / 4.0f;
	return corners + sides + center;

}

float GetInterpolateNoise(float x, float y)
{
	int intX = (int)x;
	int intY = (int)y;
	float fracX = x - intX;
	float fracY = y - intY;

	float v1 = GetSmoothNoise(intX, intY);
	float v2 = GetSmoothNoise(intX + 1, intY);
	float v3 = GetSmoothNoise(intX, intY + 1);
	float v4 = GetSmoothNoise(intX + 1, intY + 1);
	float i1 = Interpolate(v1, v2, fracX);
	float i2 = Interpolate(v3, v4, fracX);
	return Interpolate(i1, i2, fracY);
}

float GetHeight(float x, float y)
{
	float total = 0.0f;
	float d = pow(2.0f, 2.0f);
	for (int i = 0; i < 4; ++i)
	{
		float freq = (float)pow(2.0f, i) / 3.0f;
		float amp = (float)pow(0.3f, i) * 75.0f;
		total += GetInterpolateNoise(x * freq, y * freq) * 0.3f;
	}
	return total;
}
