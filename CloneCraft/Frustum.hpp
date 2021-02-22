#include <array>

class ViewFrustum {
private:
	GW::MATH::GPLANEF m_planes[6];

	enum Planes {
		Near,
		Far,
		Left,
		Right,
		Top,
		Bottom,
	};

public:
	void update(Mat4& mat) // pass projViewMatrix to this function
	{
		// left
		m_planes[Planes::Left].distance = mat.row4.data[3] + mat.row4.data[0];
		m_planes[Planes::Left].x = { mat.row1.data[3] - mat.row1.data[0] };
		m_planes[Planes::Left].y = { mat.row2.data[3] - mat.row2.data[0] };
		m_planes[Planes::Left].z = { mat.row3.data[3] - mat.row3.data[0] };

		// right
		m_planes[Planes::Right].distance = mat.row4.data[3] - mat.row4.data[0];
		m_planes[Planes::Right].x = { mat.row1.data[3] - mat.row1.data[0] };
		m_planes[Planes::Right].y = { mat.row2.data[3] - mat.row2.data[0] };
		m_planes[Planes::Right].z = { mat.row3.data[3] - mat.row3.data[0] };

		// bottom
		m_planes[Planes::Bottom].distance = mat.row4.data[3] + mat.row4.data[1];
		m_planes[Planes::Bottom].x = { mat.row1.data[3] + mat.row1.data[1] };
		m_planes[Planes::Bottom].y = { mat.row2.data[3] + mat.row2.data[1] };
		m_planes[Planes::Bottom].z = { mat.row3.data[3] - mat.row3.data[1] };

		// top
		m_planes[Planes::Top].distance = mat.row4.data[3] - mat.row4.data[1];
		m_planes[Planes::Top].x = { mat.row1.data[3] + mat.row1.data[1] };
		m_planes[Planes::Top].y = { mat.row2.data[3] + mat.row2.data[1] };
		m_planes[Planes::Top].z = { mat.row3.data[3] - mat.row3.data[1] };

		// near
		m_planes[Planes::Near].distance = mat.row4.data[3] + mat.row4.data[2];
		m_planes[Planes::Near].x = { mat.row1.data[3] + mat.row1.data[2] };
		m_planes[Planes::Near].y = { mat.row2.data[3] + mat.row2.data[2] };
		m_planes[Planes::Near].z = { mat.row3.data[3] + mat.row3.data[2] };

		// far
		m_planes[Planes::Far].distance = mat.row4.data[3] - mat.row4.data[2];
		m_planes[Planes::Far].x = { mat.row1.data[3] - mat.row1.data[2] };
		m_planes[Planes::Far].y = { mat.row2.data[3] - mat.row2.data[2] };
		m_planes[Planes::Far].z = { mat.row3.data[3] - mat.row3.data[2] };

		for (auto& plane : m_planes)
		{
			Vec4 normal( plane.x, plane.y, plane.z, 0.f );
			float length = normal.Magnitude();
			normal.x /= length;
			normal.y /= length;
			normal.z /= length;
			plane.distance /= length;
		}
	}

	bool isBoxInFrustum(AABB& box)
	{
		auto getVNegative = [&](GW::MATH::GVECTORF& normal)
		{
			GW::MATH::GVECTORF res = { box.position.x, box.position.y, box.position.z, box.position.w };

			if (normal.x < 0)
			{
				res.x += box.dimensions.x;
			}
			if (normal.y < 0)
			{
				res.y += box.dimensions.y;
			}
			if (normal.z < 0)
			{
				res.z += box.dimensions.z;
			}

			return res;
		};

		auto getVPositive = [&](GW::MATH::GVECTORF& normal)
		{
			GW::MATH::GVECTORF res = { box.position.x, box.position.y, box.position.z, box.position.w };

			if (normal.x > 0)
			{
				res.x += box.dimensions.x;
			}
			if (normal.y > 0)
			{
				res.y += box.dimensions.y;
			}
			if (normal.z > 0)
			{
				res.z += box.dimensions.z;
			}

			return res;
		};

		bool result = true;

		for (auto& plane : m_planes)
		{
			float sqDistanceToPoint;
			GW::MATH::GVECTORF normal = { plane.x, plane.y, plane.z, 0.f };
			GW::MATH::GCollision::SqDistancePointToPlaneF(getVPositive(normal), plane, sqDistanceToPoint);

			if (sqDistanceToPoint < 0)
			{
				return false;
			}
			else
			{
				GW::MATH::GCollision::SqDistancePointToPlaneF(getVNegative(normal), plane, sqDistanceToPoint);
				if (sqDistanceToPoint < 0)
				{
					result = true;
				}
			}
		}
		return result;
	}

	bool operator==(const ViewFrustum& other) const
	{
		return m_planes[0].x == other.m_planes[0].x &&
			m_planes[0].y == other.m_planes[0].y &&
			m_planes[0].z == other.m_planes[0].z &&
			m_planes[0].distance == other.m_planes[0].distance &&

			m_planes[1].x == other.m_planes[1].x &&
			m_planes[1].y == other.m_planes[1].y &&
			m_planes[1].z == other.m_planes[1].z &&
			m_planes[1].distance == other.m_planes[1].distance &&

			m_planes[2].x == other.m_planes[2].x &&
			m_planes[2].y == other.m_planes[2].y &&
			m_planes[2].z == other.m_planes[2].z &&
			m_planes[2].distance == other.m_planes[2].distance &&

			m_planes[3].x == other.m_planes[3].x &&
			m_planes[3].y == other.m_planes[3].y &&
			m_planes[3].z == other.m_planes[3].z &&
			m_planes[3].distance == other.m_planes[3].distance &&

			m_planes[4].x == other.m_planes[4].x &&
			m_planes[4].y == other.m_planes[4].y &&
			m_planes[4].z == other.m_planes[4].z &&
			m_planes[4].distance == other.m_planes[4].distance &&

			m_planes[5].x == other.m_planes[5].x &&
			m_planes[5].y == other.m_planes[5].y &&
			m_planes[5].z == other.m_planes[5].z &&
			m_planes[5].distance == other.m_planes[5].distance;
	}

	bool operator!=(const ViewFrustum& other) const
	{
		return !(*this == other);
	}
};