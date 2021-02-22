#pragma once

struct UniformBufferObject {
	alignas(16) Mat4 model;
	alignas(16) Mat4 view;
	alignas(16) Mat4 proj;
};