#ifndef UNIFORM_BUFFER_OBJECT_HPP
#define UNIFORM_BUFFER_OBJECT_HPP

struct UniformBufferObject {
	alignas(16) Mat4 model;
	alignas(16) Mat4 view;
	alignas(16) Mat4 proj;
};

#endif // UNIFORM_BUFFER_OBJECT_HPP