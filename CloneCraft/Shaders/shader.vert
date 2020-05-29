#version 450
#extension GL_ARB_separate_shader_objects : enable

// UniformBufferObject accessable as 'ubo'. these will be updated every frame to make the rectangle rotate in 3D
// The binding directive is similar to the location directive for attributes.
// referenced in the descriptor layout
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// vertex attributes. spcified per-vertex. 
// the layout(location = x) annotations assign 
// indices to the inputs that we can later use to reference them. 
// Some types, like dvec3 64 bit vectors, use multiple slots. That 
// means that the index after it must be at least 2 higher.
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}