#pragma once

template<typename T>
static inline T clamp(T value, T min, T max) {
	return value < min ? min : (value > max ? max : value);
}
