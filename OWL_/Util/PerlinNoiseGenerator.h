#pragma once

float GetValueUsingPerlinNoise(float x, float y);

DirectX::SimpleMath::Vector2 RandomGradient(int ix, int iy);

float DotGridGradient(int ix, int iy, float x, float y);

float Interpolate(float a0, float a1, float w);