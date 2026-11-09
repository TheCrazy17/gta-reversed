/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include <numbers>

#include "Quaternion.h"

void CQuaternion::InjectHooks() {
    RH_ScopedClass(CQuaternion);
    RH_ScopedCategory("Core");

    RH_ScopedOverloadedInstall(Get, "", 0x59C080, void (CQuaternion::*)(RwMatrix*) const);
    RH_ScopedOverloadedInstall(Get, "euler", 0x59C160, void (CQuaternion::*)(float*, float*, float*));
    RH_ScopedOverloadedInstall(Get, "axis_angle", 0x59C230, void (CQuaternion::*)(RwV3d*, float*));
    RH_ScopedInstall(Multiply, 0x59C270);
    RH_ScopedOverloadedInstall(Slerp, "5", 0x59C300, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float, float, float));
    RH_ScopedOverloadedInstall(Set, "matrix", 0x59C3E0, void (CQuaternion::*)(const RwMatrix&));
    RH_ScopedOverloadedInstall(Set, "euler", 0x59C530, void (CQuaternion::*)(float, float, float));
    RH_ScopedOverloadedInstall(Set, "axis_angle", 0x59C600, void (CQuaternion::*)(RwV3d*, float));
    RH_ScopedOverloadedInstall(Slerp, "3", 0x59C630, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float));
    RH_ScopedInstall(Conjugate, 0x4D37D0);
    RH_ScopedInstall(Scale, 0x4CF9B0);
    RH_ScopedInstall(Copy, 0x4CF9E0);
}

// Quat to matrix
void CQuaternion::Get(RwMatrix* out) const {
    auto vecImag2 = imag + imag;
    auto x2x = vecImag2.x * imag.x;
    auto y2x = vecImag2.y * imag.x;
    auto z2x = vecImag2.z * imag.x;

    auto y2y = vecImag2.y * imag.y;
    auto z2y = vecImag2.z * imag.y;
    auto z2z = vecImag2.z * imag.z;

    auto x2r = vecImag2.x * real;
    auto y2r = vecImag2.y * real;
    auto z2r = vecImag2.z * real;

    CVector right{1.0F - (z2z + y2y), z2r + y2x, z2x - y2r}, up{y2x - z2r, 1.0F - (z2z + x2x), x2r + z2y}, at{y2r + z2x, z2y - x2r, 1.0F - (y2y + x2x)};
    RwV3dAssign(RwMatrixGetRight(out), &right);
    RwV3dAssign(RwMatrixGetUp(out), &up);
    RwV3dAssign(RwMatrixGetAt(out), &at);
}

namespace {
// The exe uses the exact single precision values of pi, pi / 2 and 2 pi here (0x858CB8, 0x858FE4, 0x858CBC);
// the PI family in common.h is rounded to five decimals and would shift the results
constexpr float EXE_PI      = std::numbers::pi_v<float>;
constexpr float EXE_HALF_PI = std::numbers::pi_v<float> / 2.0f;
constexpr float EXE_TWO_PI  = std::numbers::pi_v<float> * 2.0f;

// 0x59C160 wraps every angle it returns into [0, 2 pi) the same way
float WrapToPositiveAngle(float angle) {
    return angle < 0.0f ? angle + EXE_TWO_PI : angle;
}
} // namespace

// Quat to euler angles
// 0x59C160
void CQuaternion::Get(float* x, float* y, float* z) {
    // Only the right, up and at vectors are written by the matrix overload, which is all this function reads
    RwMatrix m;
    Get(&m);

    // The z angle is stored (and rounded to float) before its sine and cosine are taken
    *z = WrapToPositiveAngle(std::atan2(m.right.y, m.up.y));
    const auto sinZ = std::sin(*z);
    const auto cosZ = std::cos(*z);

    *x = WrapToPositiveAngle(std::atan2(-m.at.y, sinZ * m.right.y + cosZ * m.up.y));
    *y = WrapToPositiveAngle(std::atan2(-(m.right.z * cosZ - m.up.z * sinZ), m.right.x * cosZ - m.up.x * sinZ));
}

// Quat to axis & angle
// 0x59C230
void CQuaternion::Get(RwV3d* axis, float* angle) {
    // The exe doubles the real part BEFORE taking the arc cosine (0x59C236), not the angle after it;
    // nothing in the exe calls this function, so the result is kept exactly as the exe computes it
    const auto theta = std::acos(w + w);
    *angle           = theta;

    const auto invSin = 1.0f / std::sin(theta);
    axis->x           = invSin * x;
    axis->y           = invSin * y;
    axis->z           = invSin * z;
}

// Stores result of quat multiplication
// 0x59C270
void CQuaternion::Multiply(const CQuaternion& a, const CQuaternion& b) {
    // The exe writes the vector part first and then adds to it, so the statements keep its order in case this aliases a or b
    x = b.z * a.y - a.z * b.y;
    y = a.z * b.x - a.x * b.z;
    z = a.x * b.y - b.x * a.y;

    x += b.w * a.x + a.w * b.x;
    y += a.w * b.y + b.w * a.y;
    z += a.z * b.w + a.w * b.z;
    w = a.w * b.w - (a.x * b.x + a.z * b.z + a.y * b.y);
}

// Spherical linear interpolation
// 0x59C300
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float halftheta, float sintheta_inv, float t) {
    if (halftheta == 0.0f) {
        *this = to;
        return;
    }

    // Past a quarter turn the exe interpolates towards the negated target so that the shorter arc is taken
    float scaleFrom, scaleTo;
    if (halftheta > EXE_HALF_PI) {
        halftheta = EXE_PI - halftheta;
        scaleFrom = std::sin((1.0f - t) * halftheta) * sintheta_inv;
        scaleTo   = -(std::sin(halftheta * t) * sintheta_inv);
    } else {
        scaleFrom = std::sin((1.0f - t) * halftheta) * sintheta_inv;
        scaleTo   = std::sin(halftheta * t) * sintheta_inv;
    }

    x = scaleFrom * from.x + scaleTo * to.x;
    y = scaleTo * to.y + scaleFrom * from.y;
    z = scaleTo * to.z + scaleFrom * from.z;
    w = scaleTo * to.w + scaleFrom * from.w;
}

// Quat from matrix
// 0x59C3E0
void CQuaternion::Set(const RwMatrix& m) {
    // Same four case split by the largest diagonal term as the exe, written in the exe's order of the cases
    const auto trace = m.up.y + m.right.x + m.at.z;
    if (trace >= 0.0f) {
        const auto root = std::sqrt(trace + 1.0f);
        w               = 0.5f * root;
        const auto s    = 0.5f / root;
        x               = (m.up.z - m.at.y) * s;
        y               = (m.at.x - m.right.z) * s;
        z               = (m.right.y - m.up.x) * s;
        return;
    }

    const auto diagX = m.right.x - m.up.y - m.at.z;
    if (diagX >= 0.0f) {
        const auto root = std::sqrt(diagX + 1.0f);
        x               = 0.5f * root;
        const auto s    = 0.5f / root;
        y               = (m.up.x + m.right.y) * s;
        z               = (m.at.x + m.right.z) * s;
        w               = (m.up.z - m.at.y) * s;
        return;
    }

    const auto diagY = m.up.y - m.right.x - m.at.z;
    if (diagY >= 0.0f) {
        const auto root = std::sqrt(diagY + 1.0f);
        y               = 0.5f * root;
        const auto s    = 0.5f / root;
        w               = (m.at.x - m.right.z) * s;
        x               = (m.up.x - m.right.y) * s;
        z               = (m.at.y + m.up.z) * s;
        return;
    }

    const auto root = std::sqrt(m.at.z - (m.up.y + m.right.x) + 1.0f);
    z               = 0.5f * root;
    const auto s    = 0.5f / root;
    w               = (m.right.y - m.up.x) * s;
    x               = (m.at.x + m.right.z) * s;
    y               = (m.at.y + m.up.z) * s;
}

// Quat from euler angles
// 0x59C530
void CQuaternion::Set(float x, float y, float z) {
    const auto halfX = x * 0.5f, halfY = y * 0.5f, halfZ = z * 0.5f;
    const auto cosX = std::cos(halfX), cosY = std::cos(halfY), cosZ = std::cos(halfZ);
    const auto sinX = std::sin(halfX), sinY = std::sin(halfY), sinZ = std::sin(halfZ);

    // The exe keeps these products in float temporaries, which fixes where the intermediate values are rounded
    const float cosYcosX = cosY * cosX;
    const float sinYsinX = sinY * sinX;
    const float sinXcosY = sinX * cosY;
    const float cosXsinY = cosX * sinY;

    this->w = sinYsinX * sinZ + cosYcosX * cosZ;
    this->x = cosYcosX * sinZ - sinYsinX * cosZ;
    this->y = sinZ * cosXsinY + sinXcosY * cosZ;
    this->z = cosXsinY * cosZ - sinXcosY * sinZ;
}

// Quat from axis & angle
// 0x59C600
void CQuaternion::Set(RwV3d* axis, float angle) {
    const auto halfAngle = angle * 0.5f;
    const auto s         = std::sin(halfAngle);
    x                    = s * axis->x;
    y                    = s * axis->y;
    z                    = s * axis->z;
    w                    = std::cos(halfAngle);
}

// Spherical linear interpolation
// 0x59C630
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float t) {
    // Inlined here instead of calling CAnimBlendNode::CalcTheta: the exe's 0x4D00E0 takes the two quaternions and
    // two result pointers, which is not what the member of that name is
    auto cosTheta = from.w * to.w + from.z * to.z + from.y * to.y + from.x * to.x;
    if (cosTheta > 1.0f) {
        cosTheta = 1.0f;
    }
    const auto halfTheta = std::acos(cosTheta);
    const auto invSin    = halfTheta != 0.0f ? 1.0f / std::sin(halfTheta) : 0.0f;
    Slerp(from, to, halfTheta, invSin, t);
}

// Conjugate of a quat
// 0x4D37D0
void CQuaternion::Conjugate() {
    x = -x;
    y = -y;
    z = -z;
}

// Squared length of a quat
float CQuaternion::GetLengthSquared() const {
    // Originally NOP.
    return sq(x) + sq(y) + sq(z) + sq(w);
}

// Multiplies quat by a floating point value
// 0x4CF9B0
void CQuaternion::Scale(float multiplier) {
    x *= multiplier;
    y *= multiplier;
    z *= multiplier;
    w *= multiplier;
}

// Copies value from other quat
// 0x4CF9E0
void CQuaternion::Copy(const CQuaternion& from) {
    x = from.x;
    y = from.y;
    z = from.z;
    w = from.w;
}

// Gets a dot product for quats
float CQuaternion::Dot(const CQuaternion& rhs) {
    return this->w * rhs.w + this->z * rhs.z + this->y * rhs.y + this->x * rhs.x;
}

// Normalises a quat
void CQuaternion::Normalise() {
    const auto sqMag = GetLengthSquared();
    if (sqMag == 0.f) {
        w = 1.0;
    } else {
        *this = *this / std::sqrt(sqMag);
    }
}
