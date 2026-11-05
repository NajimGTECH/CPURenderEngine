#include <stdexcept>
#include <limits>
#include <cmath>

namespace math
{
    /*
    * Implementation of the Mat4 class template.
    */

    // ----------------------------
    // Public: Constructors
    // ----------------------------

    template <typename T>
    Mat4<T>::Mat4()
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? static_cast<T>(1) : static_cast<T>(0);
    }

    template <typename T>
    Mat4<T>::Mat4(T m00, T m01, T m02, T m03,
        T m10, T m11, T m12, T m13,
        T m20, T m21, T m22, T m23,
        T m30, T m31, T m32, T m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }

    template <typename T>
    Mat4<T>::~Mat4() = default;

    // ----------------------------
    // Public: Static methods
    // ----------------------------

    template <typename T>
    Mat4<T> Mat4<T>::identity()
    {
        Mat4<T> result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = (i == j) ? static_cast<T>(1) : static_cast<T>(0);
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::zero()
    {
        Mat4<T> result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = static_cast<T>(0);
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::trs(const Vector3<T>& position, const Quaternion& rotation, const Vector3<T>& scale)
    {
        // Row vectors: p' = p * S * R * T, so the scale must be on the left of the rotation.
        // (The old code computed R * S, which rotates first and then scales along the world axes:
        //  wrong as soon as the scale is not uniform.)
        Mat4<T> rot = rotate(rotation);
        Mat4<T> scl = Mat4<T>::scale(scale);
        Mat4<T> trs = scl * rot;
        trs.m[3][0] = position.x;
        trs.m[3][1] = position.y;
        trs.m[3][2] = position.z;
        trs.m[3][3] = static_cast<T>(1);
        return trs;
    }

    template <typename T>
    Mat4<T> Mat4<T>::perspective(T fovYRadians, T aspect, T nearZ, T farZ)
    {
        T f = static_cast<T>(1) / std::tan(fovYRadians / static_cast<T>(2));
        Mat4<T> result = zero();

        result.m[0][0] = f / aspect;
        result.m[1][1] = f;
        // Row vectors: w' = x*m[0][3] + y*m[1][3] + z*m[2][3] + m[3][3], we want w' = -z,
        // so the -1 goes in m[2][3] and the depth offset in m[3][2].
        // (The old code had these two values swapped, which is the column-vector layout.)
        result.m[2][2] = (farZ + nearZ) / (nearZ - farZ);
        result.m[2][3] = static_cast<T>(-1);
        result.m[3][2] = (static_cast<T>(2) * farZ * nearZ) / (nearZ - farZ);

        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::ortho(T left, T right, T bottom, T top, T nearZ, T farZ)
    {
        Mat4<T> result = identity();
        result.m[0][0] = static_cast<T>(2) / (right - left);
        result.m[1][1] = static_cast<T>(2) / (top - bottom);
        result.m[2][2] = static_cast<T>(-2) / (farZ - nearZ);
        result.m[3][0] = -(right + left) / (right - left);
        result.m[3][1] = -(top + bottom) / (top - bottom);
        result.m[3][2] = -(farZ + nearZ) / (farZ - nearZ);
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::look_at(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up)
    {
        Vector3<T> zAxis = (eye - target).normalized();
        Vector3<T> xAxis = up.cross(zAxis).normalized();
        Vector3<T> yAxis = zAxis.cross(xAxis);

        Mat4<T> result = identity();
        result.m[0][0] = xAxis.x; result.m[0][1] = yAxis.x; result.m[0][2] = zAxis.x;
        result.m[1][0] = xAxis.y; result.m[1][1] = yAxis.y; result.m[1][2] = zAxis.y;
        result.m[2][0] = xAxis.z; result.m[2][1] = yAxis.z; result.m[2][2] = zAxis.z;
        result.m[3][0] = -xAxis.dot(eye);
        result.m[3][1] = -yAxis.dot(eye);
        result.m[3][2] = -zAxis.dot(eye);
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::translate(const Vector3<T>& position)
    {
        Mat4<T> result = identity();
        result.m[3][0] = position.x;
        result.m[3][1] = position.y;
        result.m[3][2] = position.z;
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::scale(const Vector3<T>& scale)
    {
        Mat4<T> result = identity();
        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::rotate(const Quaternion& rotation)
    {
        Mat4<T> result = identity();
        T xx = static_cast<T>(rotation.x) * static_cast<T>(rotation.x);
        T yy = static_cast<T>(rotation.y) * static_cast<T>(rotation.y);
        T zz = static_cast<T>(rotation.z) * static_cast<T>(rotation.z);
        T xy = static_cast<T>(rotation.x) * static_cast<T>(rotation.y);
        T xz = static_cast<T>(rotation.x) * static_cast<T>(rotation.z);
        T yz = static_cast<T>(rotation.y) * static_cast<T>(rotation.z);
        T wx = static_cast<T>(rotation.w) * static_cast<T>(rotation.x);
        T wy = static_cast<T>(rotation.w) * static_cast<T>(rotation.y);
        T wz = static_cast<T>(rotation.w) * static_cast<T>(rotation.z);

        result.m[0][0] = static_cast<T>(1) - static_cast<T>(2) * (yy + zz);
        result.m[0][1] = static_cast<T>(2) * (xy + wz);
        result.m[0][2] = static_cast<T>(2) * (xz - wy);
        result.m[1][0] = static_cast<T>(2) * (xy - wz);
        result.m[1][1] = static_cast<T>(1) - static_cast<T>(2) * (xx + zz);
        result.m[1][2] = static_cast<T>(2) * (yz + wx);
        result.m[2][0] = static_cast<T>(2) * (xz + wy);
        result.m[2][1] = static_cast<T>(2) * (yz - wx);
        result.m[2][2] = static_cast<T>(1) - static_cast<T>(2) * (xx + yy);
        return result;
    }

    // ----------------------------
    // Public: Operations
    // ----------------------------

    template <typename T>
    Mat4<T> Mat4<T>::transpose() const
    {
        Mat4<T> result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.m[i][j] = m[j][i];
        return result;
    }

    template <typename T>
    Mat4<T> Mat4<T>::inverse() const
    {
        std::array<T, 16> inv;
        T det;

        inv[0] = m[1][1] * m[2][2] * m[3][3] -
            m[1][1] * m[2][3] * m[3][2] -
            m[2][1] * m[1][2] * m[3][3] +
            m[2][1] * m[1][3] * m[3][2] +
            m[3][1] * m[1][2] * m[2][3] -
            m[3][1] * m[1][3] * m[2][2];

        inv[4] = -m[1][0] * m[2][2] * m[3][3] +
            m[1][0] * m[2][3] * m[3][2] +
            m[2][0] * m[1][2] * m[3][3] -
            m[2][0] * m[1][3] * m[3][2] -
            m[3][0] * m[1][2] * m[2][3] +
            m[3][0] * m[1][3] * m[2][2];

        inv[8] = m[1][0] * m[2][1] * m[3][3] -
            m[1][0] * m[2][3] * m[3][1] -
            m[2][0] * m[1][1] * m[3][3] +
            m[2][0] * m[1][3] * m[3][1] +
            m[3][0] * m[1][1] * m[2][3] -
            m[3][0] * m[1][3] * m[2][1];

        inv[12] = -m[1][0] * m[2][1] * m[3][2] +
            m[1][0] * m[2][2] * m[3][1] +
            m[2][0] * m[1][1] * m[3][2] -
            m[2][0] * m[1][2] * m[3][1] -
            m[3][0] * m[1][1] * m[2][2] +
            m[3][0] * m[1][2] * m[2][1];

        inv[1] = -m[0][1] * m[2][2] * m[3][3] +
            m[0][1] * m[2][3] * m[3][2] +
            m[2][1] * m[0][2] * m[3][3] -
            m[2][1] * m[0][3] * m[3][2] -
            m[3][1] * m[0][2] * m[2][3] +
            m[3][1] * m[0][3] * m[2][2];

        inv[5] = m[0][0] * m[2][2] * m[3][3] -
            m[0][0] * m[2][3] * m[3][2] -
            m[2][0] * m[0][2] * m[3][3] +
            m[2][0] * m[0][3] * m[3][2] +
            m[3][0] * m[0][2] * m[2][3] -
            m[3][0] * m[0][3] * m[2][2];

        inv[9] = -m[0][0] * m[2][1] * m[3][3] +
            m[0][0] * m[2][3] * m[3][1] +
            m[2][0] * m[0][1] * m[3][3] -
            m[2][0] * m[0][3] * m[3][1] -
            m[3][0] * m[0][1] * m[2][3] +
            m[3][0] * m[0][3] * m[2][1];

        inv[13] = m[0][0] * m[2][1] * m[3][2] -
            m[0][0] * m[2][2] * m[3][1] -
            m[2][0] * m[0][1] * m[3][2] +
            m[2][0] * m[0][2] * m[3][1] +
            m[3][0] * m[0][1] * m[2][2] -
            m[3][0] * m[0][2] * m[2][1];

        inv[2] = m[0][1] * m[1][2] * m[3][3] -
            m[0][1] * m[1][3] * m[3][2] -
            m[1][1] * m[0][2] * m[3][3] +
            m[1][1] * m[0][3] * m[3][2] +
            m[3][1] * m[0][2] * m[1][3] -
            m[3][1] * m[0][3] * m[1][2];

        inv[6] = -m[0][0] * m[1][2] * m[3][3] +
            m[0][0] * m[1][3] * m[3][2] +
            m[1][0] * m[0][2] * m[3][3] -
            m[1][0] * m[0][3] * m[3][2] -
            m[3][0] * m[0][2] * m[1][3] +
            m[3][0] * m[0][3] * m[1][2];

        inv[10] = m[0][0] * m[1][1] * m[3][3] -
            m[0][0] * m[1][3] * m[3][1] -
            m[1][0] * m[0][1] * m[3][3] +
            m[1][0] * m[0][3] * m[3][1] +
            m[3][0] * m[0][1] * m[1][3] -
            m[3][0] * m[0][3] * m[1][1];

        inv[14] = -m[0][0] * m[1][1] * m[3][2] +
            m[0][0] * m[1][2] * m[3][1] +
            m[1][0] * m[0][1] * m[3][2] -
            m[1][0] * m[0][2] * m[3][1] -
            m[3][0] * m[0][1] * m[1][2] +
            m[3][0] * m[0][2] * m[1][1];

        inv[3] = -m[0][1] * m[1][2] * m[2][3] +
            m[0][1] * m[1][3] * m[2][2] +
            m[1][1] * m[0][2] * m[2][3] -
            m[1][1] * m[0][3] * m[2][2] -
            m[2][1] * m[0][2] * m[1][3] +
            m[2][1] * m[0][3] * m[1][2];

        inv[7] = m[0][0] * m[1][2] * m[2][3] -
            m[0][0] * m[1][3] * m[2][2] -
            m[1][0] * m[0][2] * m[2][3] +
            m[1][0] * m[0][3] * m[2][2] +
            m[2][0] * m[0][2] * m[1][3] -
            m[2][0] * m[0][3] * m[1][2];

        inv[11] = -m[0][0] * m[1][1] * m[2][3] +
            m[0][0] * m[1][3] * m[2][1] +
            m[1][0] * m[0][1] * m[2][3] -
            m[1][0] * m[0][3] * m[2][1] -
            m[2][0] * m[0][1] * m[1][3] +
            m[2][0] * m[0][3] * m[1][1];

        inv[15] = m[0][0] * m[1][1] * m[2][2] -
            m[0][0] * m[1][2] * m[2][1] -
            m[1][0] * m[0][1] * m[2][2] +
            m[1][0] * m[0][2] * m[2][1] +
            m[2][0] * m[0][1] * m[1][2] -
            m[2][0] * m[0][2] * m[1][1];

        det = m[0][0] * inv[0] + m[0][1] * inv[4] + m[0][2] * inv[8] + m[0][3] * inv[12];

        if (det == 0)
            throw std::runtime_error("Matrix is singular and cannot be inverted.");

        det = static_cast<T>(1) / det;

        Mat4<T> result;
        for (int i = 0; i < 16; i++)
            result.m[i / 4][i % 4] = inv[i] * det;

        return result;
    }

    template <typename T>
    T& Mat4<T>::operator()(int row, int col)
    {
        return m[row][col];
    }

    template <typename T>
    const T& Mat4<T>::operator()(int row, int col) const
    {
        return m[row][col];
    }

    template <typename T>
    Vector3<T> Mat4<T>::extract_position() const
    {
        return Vector3<T>(m[3][0], m[3][1], m[3][2]);
    }

    template <typename T>
    void Mat4<T>::set_position(const Vector3<T>& position)
    {
        m[3][0] = position.x;
        m[3][1] = position.y;
        m[3][2] = position.z;
    }

    template <typename T>
    Vector3<T> Mat4<T>::extract_scale() const
    {
        Vector3<T> scale;
        scale.x = Vector3<T>(m[0][0], m[0][1], m[0][2]).length();
        scale.y = Vector3<T>(m[1][0], m[1][1], m[1][2]).length();
        scale.z = Vector3<T>(m[2][0], m[2][1], m[2][2]).length();
        return scale;
    }

    template <typename T>
    Vector3<T> Mat4<T>::get_row(int row) const
    {
        return Vector3<T>(
            m[row][0],
            m[row][1],
            m[row][2]
        );
    }

    template <typename T>
    Vector3<T> Mat4<T>::get_column(int col) const
    {
        return Vector3<T>(
            m[0][col],
            m[1][col],
            m[2][col]
        );
    }

    // ----------------------------
    // Public: Missing operations implemented
    // ----------------------------

    template <typename T>
    Mat4<T> Mat4<T>::operator*(const Mat4<T>& other) const
    {
        Mat4<T> result = Mat4<T>::zero();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                T sum = static_cast<T>(0);
                for (int k = 0; k < 4; ++k)
                    sum += m[i][k] * other.m[k][j];
                result.m[i][j] = sum;
            }
        }
        return result;
    }

    template <typename T>
    Vector3<T> Mat4<T>::multiply_point(const Vector3<T>& v) const
    {
        T x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
        T y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
        T z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
        T w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + m[3][3];

        if (w != static_cast<T>(0) && w != static_cast<T>(1))
        {
            x /= w;
            y /= w;
            z /= w;
        }

        return Vector3<T>(x, y, z);
    }

    template <typename T>
    Vector3<T> Mat4<T>::multiply_point_affine(const Vector3<T>& v) const
    {
        return Vector3<T>(
            v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0],
            v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1],
            v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2]
        );
    }

    template <typename T>
    Vector3<T> Mat4<T>::multiply_vector(const Vector3<T>& v) const
    {
        // Same row-vector convention as multiply_point, but with w = 0 (no translation).
        // (The old code used m[0][1], m[0][2] for x', i.e. the column-vector convention.)
        T x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0];
        T y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1];
        T z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2];
        return Vector3<T>(x, y, z);
    }

    template <typename T>
    T Mat4<T>::determinant() const
    {
        // Compute determinant via expansion by minors (3x3 determinants).
        auto det3 = [](T a1, T a2, T a3,
            T b1, T b2, T b3,
            T c1, T c2, T c3) -> T
            {
                return a1 * (b2 * c3 - b3 * c2)
                    - a2 * (b1 * c3 - b3 * c1)
                    + a3 * (b1 * c2 - b2 * c1);
            };

        T det =
            m[0][0] * det3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3])
            - m[0][1] * det3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3])
            + m[0][2] * det3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3])
            - m[0][3] * det3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);

        return det;
    }

    template <typename T>
    Quaternion Mat4<T>::extract_rotation() const
    {
        // Convert upper-left 3x3 to quaternion using standard matrix-to-quaternion conversion.
        // Assumes matrix is rotation (possibly with scale). We remove scale first.
        //
        // With row vectors, row i of the matrix is (scale_i * rotated axis i). The textbook formula
        // below is written for the column-vector rotation matrix R, which is the transpose of ours:
        // r_ij = m[j][i] / scale_j.
        // (The old code divided by the column scale and did not transpose, so it returned the
        //  inverse rotation.)
        Vector3<T> scale = extract_scale();
        T sx = scale.x, sy = scale.y, sz = scale.z;
        T invSx = (sx != static_cast<T>(0)) ? static_cast<T>(1) / sx : static_cast<T>(1);
        T invSy = (sy != static_cast<T>(0)) ? static_cast<T>(1) / sy : static_cast<T>(1);
        T invSz = (sz != static_cast<T>(0)) ? static_cast<T>(1) / sz : static_cast<T>(1);

        T r00 = m[0][0] * invSx;
        T r01 = m[1][0] * invSy;
        T r02 = m[2][0] * invSz;
        T r10 = m[0][1] * invSx;
        T r11 = m[1][1] * invSy;
        T r12 = m[2][1] * invSz;
        T r20 = m[0][2] * invSx;
        T r21 = m[1][2] * invSy;
        T r22 = m[2][2] * invSz;

        T trace = r00 + r11 + r22;
        T qw, qx, qy, qz;

        if (trace > static_cast<T>(0))
        {
            T s = static_cast<T>(std::sqrt(static_cast<double>(trace) + 1.0)) * static_cast<T>(2);
            qw = static_cast<T>(0.25) * s;
            qx = (r21 - r12) / s;
            qy = (r02 - r20) / s;
            qz = (r10 - r01) / s;
        }
        else if ((r00 > r11) && (r00 > r22))
        {
            T s = static_cast<T>(std::sqrt(static_cast<double>(1.0 + r00 - r11 - r22))) * static_cast<T>(2);
            qw = (r21 - r12) / s;
            qx = static_cast<T>(0.25) * s;
            qy = (r01 + r10) / s;
            qz = (r02 + r20) / s;
        }
        else if (r11 > r22)
        {
            T s = static_cast<T>(std::sqrt(static_cast<double>(1.0 + r11 - r00 - r22))) * static_cast<T>(2);
            qw = (r02 - r20) / s;
            qx = (r01 + r10) / s;
            qy = static_cast<T>(0.25) * s;
            qz = (r12 + r21) / s;
        }
        else
        {
            T s = static_cast<T>(std::sqrt(static_cast<double>(1.0 + r22 - r00 - r11))) * static_cast<T>(2);
            qw = (r10 - r01) / s;
            qx = (r02 + r20) / s;
            qy = (r12 + r21) / s;
            qz = static_cast<T>(0.25) * s;
        }

        // Quaternion class uses floats; cast safely.
        return Quaternion(static_cast<float>(qw), static_cast<float>(qx), static_cast<float>(qy), static_cast<float>(qz));
    }

    template <typename T>
    bool Mat4<T>::valid_trs() const
    {
        // Check last row is (0,0,0,1) and upper-left 3x3 is orthonormal up to scale.
        const double EPS = 1e-5;
        if (std::fabs(static_cast<double>(m[0][3])) > EPS) return false;
        if (std::fabs(static_cast<double>(m[1][3])) > EPS) return false;
        if (std::fabs(static_cast<double>(m[2][3])) > EPS) return false;
        if (std::fabs(static_cast<double>(m[3][3] - static_cast<T>(1))) > EPS) return false;

        // Row vectors: the axes of the transform are the rows 0, 1, 2 (not the columns).
        Vector3<T> row0 = get_row(0);
        Vector3<T> row1 = get_row(1);
        Vector3<T> row2 = get_row(2);

        // Non-zero scale?
        Vector3<T> scale = extract_scale();
        if (scale.x == static_cast<T>(0) || scale.y == static_cast<T>(0) || scale.z == static_cast<T>(0))
            return false;

        // Normalize the axes and check that they are orthogonal
        Vector3<T> n0 = row0.normalized();
        Vector3<T> n1 = row1.normalized();
        Vector3<T> n2 = row2.normalized();

        double d01 = std::fabs(static_cast<double>(n0.dot(n1)));
        double d02 = std::fabs(static_cast<double>(n0.dot(n2)));
        double d12 = std::fabs(static_cast<double>(n1.dot(n2)));

        if (d01 > EPS || d02 > EPS || d12 > EPS) return false;

        return true;
    }

    template <typename T>
    bool Mat4<T>::operator==(const Mat4<T>& other) const
    {
        const double EPS = 1e-6;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (std::fabs(static_cast<double>(m[i][j] - other.m[i][j])) > EPS)
                    return false;
        return true;
    }

    template <typename T>
    bool Mat4<T>::operator!=(const Mat4<T>& other) const
    {
        return !(*this == other);
    }

}