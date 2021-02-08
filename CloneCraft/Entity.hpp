#pragma once


struct Entity 
{
	GW::MATH::GVECTORF position;
	GW::MATH::GVECTORF rotation;
	GW::MATH::GVECTORF velocity;
	GW::MATH::GAABBMMF bbox;

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

	Entity(const GW::MATH::GVECTORF& pos, const GW::MATH::GVECTORF& rot, const GW::MATH::GAABBMMF& _bbox) :
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
				bbox.min.x == other.bbox.min.x &&
				bbox.min.y == other.bbox.min.y &&
				bbox.min.z == other.bbox.min.z &&
				bbox.min.w == other.bbox.min.w &&
				bbox.max.x == other.bbox.max.x &&
				bbox.max.y == other.bbox.max.y &&
				bbox.max.z == other.bbox.max.z &&
				bbox.max.w == other.bbox.max.w);
	}

	bool operator!=(const Entity& other) const
	{
		return !(*this == other);
	}
};