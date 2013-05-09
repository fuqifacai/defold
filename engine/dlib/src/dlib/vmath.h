#ifndef DM_VMATH_H
#define DM_VMATH_H

#include <vectormath/cpp/vectormath_aos.h>

#include "math.h"
#include "trig_lookup.h"

/**
 * Collection of vector math functions.
 */
namespace dmVMath
{
    /**
     * Construct a quaternion from an axis index and angle
     * @param axis_index Index of the axis the quaternion should be a rotation around
     * @param radians Angle of rotation
     * @return Quaternion describing the rotation
     */
    inline Vectormath::Aos::Quat QuatFromAngle(uint32_t axis_index, float radians)
    {
        Vectormath::Aos::Quat q(0, 0, 0, 1);
        float half_angle = 0.5f * radians;
        q.setElem(axis_index, dmTrigLookup::Sin(half_angle));
        q.setW(dmTrigLookup::Cos(half_angle));
        return q;
    }

#define DEG_FACTOR 57.295779513f

    /**
     * Converts a quaternion into euler angles (r0, r1, r2), based on the specified rotation order.
     * Order XYZ is specified as (x, y, z, w)
     * To handle gimbal lock (singularity at r1 ~ +/- 90 degrees), the cut off is at r0 = +/- 88.85 degrees, which snaps to +/- 90.
     * The provided quaternion is expected to be normalized.
     * The error is guaranteed to be less than +/- 0.02 degrees
     * @param q0 first imaginary axis
     * @param q1 second imaginary axis
     * @param q2 third imaginary axis
     * @param q3 real part
     * @result Euler angles in degrees and the same order as the specified rotation order
     */
    inline Vectormath::Aos::Vector3 QuatToEuler(float q0, float q1, float q2, float q3)
    {
        // Implementation based on:
        // * http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // * http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
        // * http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/Quaternions.pdf
        const float limit = 0.4999f; // gimbal lock limit, corresponds to 88.85 degrees
        float test = q3 * q0 + q1 * q2;
        float r0, r1, r2;
        if (test > limit)
        {
            r0 = 2.0f * atan2(q0, q3);
            r1 = M_PI_2;
            r2 = 0.0f;
        }
        else if (test < -limit)
        {
            r0 = -2.0f * atan2(q0, q3);
            r1 = -M_PI_2;
            r2 = 0.0f;
        }
        else
        {
            float sq0 = q0 * q0;
            float sq1 = q1 * q1;
            float sq2 = q2 * q2;
            r0 = atan2(2.0f * (q3 * q0 - q1 * q2), 1.0f - 2.0f * (sq0 + sq1));
            r1 = asin(2.0f * (q3 * q1 + q0 * q2));
            r2 = atan2(2.0f * (q3 * q2 - q0 * q1), 1.0f - 2.0f * (sq1 + sq2));
        }
        return Vectormath::Aos::Vector3(r0, r1, r2) * DEG_FACTOR;
    }

#undef DEG_FACTOR
#define HALF_RAD_FACTOR 0.008726646f

    /**
     * Converts euler angles (x, y, z) in degrees into a quaternion.
     * The provided quaternion is expected to be normalized.
     * The error is guaranteed to be less than 0.001.
     * @param x rotation around x-axis (deg)
     * @param y rotation around y-axis (deg)
     * @param z rotation around z-axis (deg)
     * @result Quat describing an equivalent rotation
     */
    inline Vectormath::Aos::Quat EulerToQuat(Vectormath::Aos::Vector3 xyz)
    {
        float x = xyz.getX() * HALF_RAD_FACTOR;
        float y = xyz.getY() * HALF_RAD_FACTOR;
        float z = xyz.getZ() * HALF_RAD_FACTOR;
        float sx = dmTrigLookup::Sin(x);
        float cx = dmTrigLookup::Cos(x);
        float sy = dmTrigLookup::Sin(y);
        float cy = dmTrigLookup::Cos(y);
        float sz = dmTrigLookup::Sin(z);
        float cz = dmTrigLookup::Cos(z);
        float cxcy = cx * cy;
        float sxsy = sx * sy;
        return Vectormath::Aos::Quat(
            sx * cy * cz + cx * sy * sz,
            cx * sy * cz  - sx * cy * sz,
            cxcy * sz - sxsy * cz,
            cxcy * cz + sxsy * sz
        );
    }

#undef HALF_RAD_FACTOR

}

#endif
