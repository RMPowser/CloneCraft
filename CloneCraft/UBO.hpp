#pragma once

struct UniformBufferObject {
	alignas(16) GW::MATH::GMATRIXF model;
	alignas(16) GW::MATH::GMATRIXF view;
	alignas(16) GW::MATH::GMATRIXF proj;
};