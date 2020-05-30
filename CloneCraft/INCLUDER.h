#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// The glm/gtc/matrix_transform.hpp header exposes functions that can be used to generate model transformations such as glm::rotate,
// view transformations such as glm::lookAt, and projection transformations such as glm::perspective. The GLM_FORCE_RADIANS 
// definition is necessary to make sure that functions like glm::rotate use radians as arguments, to avoid any possible confusion.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // force GLM to use a version of its types that has the alignment requirements already specified for us
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION // The header only defines the prototypes of the functions by default. One code file needs to include the header with the STB_IMAGE_IMPLEMENTATION definition to include the function bodies, otherwise we'll get linking errors.
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// The chrono standard library header exposes functions to do precise timekeeping. We'll use this to make sure that the geometry
// rotates 90 degrees per second regardless of frame rate.
#include <chrono>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm>
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define PAUSE system("pause")