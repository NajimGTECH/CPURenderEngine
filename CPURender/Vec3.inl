namespace math
{

    // Constructors
    template<typename T>
    constexpr Vector3<T>::Vector3() : x(0), y(0), z(0) {}

    template<typename T>
    constexpr Vector3<T>::Vector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    template<typename T>
    template<typename U>
    constexpr Vector3<T>::Vector3(const Vector3<U>& other)
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {
    }

    // Arithmetic operators
    template<typename T>
    constexpr Vector3<T> Vector3<T>::operator+(const Vector3& rhs) const
    {
        return { x + rhs.x, y + rhs.y, z + rhs.z };
    }

    template<typename T>
    constexpr Vector3<T> Vector3<T>::operator-(const Vector3& rhs) const
    {
        return { x - rhs.x, y - rhs.y, z - rhs.z };
    }

    template<typename T>
    constexpr Vector3<T>& Vector3<T>::operator+=(const Vector3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    template<typename T>
    constexpr Vector3<T>& Vector3<T>::operator-=(const Vector3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    template<typename T>
    constexpr Vector3<T> Vector3<T>::operator*(T scalar) const
    {
        return { x * scalar, y * scalar, z * scalar };
    }

    template<typename T>
    constexpr Vector3<T> Vector3<T>::operator/(T scalar) const
    {
        return { x / scalar, y / scalar, z / scalar };
    }

    template<typename T>
    constexpr Vector3<T>& Vector3<T>::operator*=(T scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    template<typename T>
    constexpr Vector3<T>& Vector3<T>::operator/=(T scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Comparison
    template<typename T>
    constexpr bool Vector3<T>::equals(const Vector3& rhs, T epsilon) const
    {
        return std::fabs(x - rhs.x) < epsilon &&
            std::fabs(y - rhs.y) < epsilon &&
            std::fabs(z - rhs.z) < epsilon;
    }

    template<typename T>
    constexpr bool Vector3<T>::operator==(const Vector3& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    template<typename T>
    constexpr bool Vector3<T>::operator!=(const Vector3& rhs) const
    {
        return !(*this == rhs);
    }

    // Dot and Cross products
    template<typename T>
    constexpr T Vector3<T>::dot(const Vector3& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    template<typename T>
    constexpr Vector3<T> Vector3<T>::cross(const Vector3& rhs) const
    {
        return Vector3<T>(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    // Magnitude and normalization
    template<typename T>
    T Vector3<T>::length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    template<typename T>
    T Vector3<T>::length_squared() const
    {
        return x * x + y * y + z * z;
    }

    template<typename T>
    Vector3<T> Vector3<T>::normalized() const
    {
        T len = length();
        return len > static_cast<T>(0) ? (*this / len) : Vector3<T>(0, 0, 0);
    }

    template<typename T>
    void Vector3<T>::normalize()
    {
        // Same rule as normalized(): a length that is not > 0 (zero or NaN) gives (0, 0, 0).
        *this = normalized();
    }

    // Distance and interpolation
    template<typename T>
    T Vector3<T>::distance(const Vector3& a, const Vector3& b)
    {
        return (a - b).length();
    }

    template<typename T>
    Vector3<T> Vector3<T>::lerp(const Vector3& a, const Vector3& b, T t)
    {
        return a + (b - a) * t;
    }

    // Utility
    template<typename T>
    Vector3<T> Vector3<T>::perpendicular() const
    {
        // Returns a perpendicular vector (arbitrary, useful for generating orthogonal basis)
        return std::fabs(x) > std::fabs(z) ? Vector3<T>(-y, x, 0) : Vector3<T>(0, -z, y);
    }

    template<typename T>
    Vector3<T> Vector3<T>::scale(const Vector3& a, const Vector3& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    template<typename T>
    Vector3<T> Vector3<T>::min(const Vector3& a, const Vector3& b)
    {
        return {
            (a.x < b.x) ? a.x : b.x,
            (a.y < b.y) ? a.y : b.y,
            (a.z < b.z) ? a.z : b.z
        };
    }

    template<typename T>
    Vector3<T> Vector3<T>::max(const Vector3& a, const Vector3& b)
    {
        return {
            (a.x > b.x) ? a.x : b.x,
            (a.y > b.y) ? a.y : b.y,
            (a.z > b.z) ? a.z : b.z
        };
    }

    template<typename T>
    Vector3<T> Vector3<T>::move_towards(const Vector3& current, const Vector3& target, T maxDistanceDelta)
    {
        Vector3<T> delta = target - current;
        T distance = delta.length();
        if (distance <= maxDistanceDelta || distance == static_cast<T>(0))
            return target;

        return current + delta / distance * maxDistanceDelta;
    }

    template<typename T>
    Vector3<T> Vector3<T>::reflect(const Vector3& direction, const Vector3& normal)
    {
        return direction - normal * (static_cast<T>(2) * direction.dot(normal));
    }

    template<typename T>
    T Vector3<T>::angle(const Vector3& a, const Vector3& b)
    {
        T dot = a.dot(b);
        T mag = a.length() * b.length();
        if (mag == static_cast<T>(0))
            return static_cast<T>(0);

        T cosTheta = dot / mag;
        cosTheta = std::max(static_cast<T>(-1), std::min(static_cast<T>(1), cosTheta));
        return std::acos(cosTheta) * static_cast<T>(180.0 / std::numbers::pi_v<float>);
    }
}