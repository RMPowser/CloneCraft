#pragma once


struct Entity 
{
	GW::MATH::GVECTORF position;
	GW::MATH::GVECTORF rotation;
	GW::MATH::GVECTORF velocity;
	GW::MATH::GAABBCEF bbox;

	Entity() :
		bbox({ 0, 0, 0, 0, 0, 0 }),
		position(GW::MATH::GZeroVectorF),
		rotation(GW::MATH::GZeroVectorF),
		velocity(GW::MATH::GZeroVectorF){
	}

	Entity(const GW::MATH::GVECTORF pos, const GW::MATH::GVECTORF& rot) :
		bbox({ 0, 0, 0, 0, 0, 0 }),
		position(pos),
		rotation(rot),
		velocity(GW::MATH::GZeroVectorF) {
	}

	Entity(const GW::MATH::GVECTORF& pos, const GW::MATH::GVECTORF& rot, const GW::MATH::GAABBCEF& _bbox) :
		bbox(_bbox),
		position(pos),
		rotation(rot),
		velocity(GW::MATH::GZeroVectorF) {
	}

	bool operator==(const Entity& other) const
	{
		return (position.x == other.position.x &&
				position.y == other.position.y &&
				position.z == other.position.z &&
				position.w == other.position.w &&
				rotation.x == other.rotation.x &&
				rotation.y == other.rotation.y &&
				rotation.z == other.rotation.z &&
				rotation.w == other.rotation.w &&
				velocity.x == other.velocity.x &&
				velocity.y == other.velocity.y &&
				velocity.z == other.velocity.z &&
				velocity.w == other.velocity.w &&
				bbox.center.x == other.bbox.center.x &&
				bbox.center.y == other.bbox.center.y &&
				bbox.center.z == other.bbox.center.z &&
				bbox.center.w == other.bbox.center.w &&
				bbox.extent.x == other.bbox.extent.x &&
				bbox.extent.y == other.bbox.extent.y &&
				bbox.extent.z == other.bbox.extent.z &&
				bbox.extent.w == other.bbox.extent.w);
	}

	bool operator!=(const Entity& other) const
	{
		return !(*this == other);
	}
};