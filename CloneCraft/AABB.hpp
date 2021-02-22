#pragma once

class AABB {
public:
    Vec4 position{ 0, 0, 0, 0 };
    Vec4 dimensions{ 0, 0, 0, 0 };

    AABB() {};

    AABB(const Vec4& pos) {
        position = pos;
    }

    AABB(const Vec4& pos, const Vec4& dim) {
        position = pos;
        dimensions = dim;
    }

    bool operator==(const AABB& other) const {
        return position == other.position && dimensions == other.dimensions;
    }

    bool operator!=(const AABB& other) const {
        return !(*this == other);
    }
};

