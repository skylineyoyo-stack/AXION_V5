// Minimal Madgwick AHRS (IMU-only)
#pragma once

#include <math.h>

struct Madgwick {
  float beta = 0.1f; // algorithm gain
  float q0=1, q1=0, q2=0, q3=0;

  void updateIMU(float gx, float gy, float gz, float ax, float ay, float az, float dt) {
    float q0q0 = q0*q0, q0q1=q0*q1, q0q2=q0*q2, q0q3=q0*q3;
    float q1q1=q1*q1, q1q2=q1*q2, q1q3=q1*q3;
    float q2q2=q2*q2, q2q3=q2*q3;
    float q3q3=q3*q3;

    // Normalise accelerometer
    float norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm <= 1e-6f) return;
    ax /= norm; ay /= norm; az /= norm;

    // Auxiliary variables to avoid repeated arithmetic
    float _2q0 = 2.0f * q0;
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _4q0 = 4.0f * q0;
    float _4q1 = 4.0f * q1;
    float _4q2 = 4.0f * q2;
    float _8q1 = 8.0f * q1;
    float _8q2 = 8.0f * q2;
    float q0q0_ = q0*q0;
    float q1q1_ = q1*q1;
    float q2q2_ = q2*q2;
    float q3q3_ = q3*q3;

    // Gradient decent algorithm corrective step
    float s0 = _4q0*q2q2_ + _2q2*ax + _4q0*q1q1_ - _2q1*ay;
    float s1 = _4q1*q3q3_ - _2q3*ax + 4.0f*q0q0_*q1 - _2q0*ay - _4q1 + _8q1*q1q1_ + _8q1*q2q2_ + _4q1*az;
    float s2 = 4.0f*q0q0_*q2 + _2q0*ax + _4q2*q3q3_ - _2q3*ay - _4q2 + _8q2*q1q1_ + _8q2*q2q2_ + _4q2*az;
    float s3 = 4.0f*q1q1_*q3 - _2q1*ax + 4.0f*q2q2_*q3 - _2q2*ay;
    norm = 1.0f / sqrtf(s0*s0 + s1*s1 + s2*s2 + s3*s3);
    s0 *= norm; s1 *= norm; s2 *= norm; s3 *= norm;

    // Rate of change of quaternion
    float qDot0 = 0.5f * (-q1*gx - q2*gy - q3*gz) - beta*s0;
    float qDot1 = 0.5f * ( q0*gx + q2*gz - q3*gy) - beta*s1;
    float qDot2 = 0.5f * ( q0*gy - q1*gz + q3*gx) - beta*s2;
    float qDot3 = 0.5f * ( q0*gz + q1*gy - q2*gx) - beta*s3;

    // Integrate to yield quaternion
    q0 += qDot0 * dt;
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;

    // Normalize quaternion
    norm = 1.0f / sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    q0 *= norm; q1 *= norm; q2 *= norm; q3 *= norm;
  }

  void getEuler(float& yaw, float& pitch, float& roll) const {
    // yaw (psi), pitch (theta), roll (phi)
    float ys = 2.0f*(q0*q3 + q1*q2);
    float yc = 1.0f - 2.0f*(q2*q2 + q3*q3);
    yaw = atan2f(ys, yc) * 57.2957795f;
    float ps = 2.0f*(q0*q2 - q3*q1);
    ps = ps > 1 ? 1 : (ps < -1 ? -1 : ps);
    pitch = asinf(ps) * 57.2957795f;
    float rs = 2.0f*(q0*q1 + q2*q3);
    float rc = 1.0f - 2.0f*(q1*q1 + q2*q2);
    roll = atan2f(rs, rc) * 57.2957795f;
  }

  // Full update with magnetometer
  void update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz, float dt) {
    float q0q0 = q0*q0, q0q1=q0*q1, q0q2=q0*q2, q0q3=q0*q3;
    float q1q1=q1*q1, q1q2=q1*q2, q1q3=q1*q3;
    float q2q2=q2*q2, q2q3=q2*q3;
    float q3q3=q3*q3;

    // Normalise accelerometer
    float norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm <= 1e-6f) return;
    ax /= norm; ay /= norm; az /= norm;

    // Normalise magnetometer
    norm = sqrtf(mx*mx + my*my + mz*mz);
    if (norm <= 1e-6f) { updateIMU(gx,gy,gz,ax,ay,az,dt); return; }
    mx /= norm; my /= norm; mz /= norm;

    // Reference direction of Earth's magnetic field
    float _2q0mx = 2.0f*q0*mx;
    float _2q0my = 2.0f*q0*my;
    float _2q0mz = 2.0f*q0*mz;
    float _2q1mx = 2.0f*q1*mx;
    float hx = mx*q0q0 - _2q0my*q3 + _2q0mz*q2 + mx*q1q1 + 2.0f*q1*my*q2 + 2.0f*q1*mz*q3 - mx*q2q2 - mx*q3q3;
    float hy = _2q0mx*q3 + my*q0q0 - _2q0mz*q1 + _2q1mx*q2 - my*q1q1 + my*q2q2 + 2.0f*q2*mz*q3 - my*q3q3;
    float _2bx = sqrtf(hx*hx + hy*hy);
    float _2bz = -_2q0mx*q2 + _2q0my*q1 + mz*q0q0 + _2q1mx*q3 - mz*q1q1 + 2.0f*q2*my*q3 - mz*q2q2 + mz*q3q3;
    float _4bx = 2.0f*_2bx;
    float _4bz = 2.0f*_2bz;

    // Gradient decent algorithm corrective step
    float s0 = -2.0f*(q2*(2.0f*q1q3 - 2.0f*q0q2 - ax) + q1*(2.0f*q0q1 + 2.0f*q2q3 - ay))
               - _2bz*q2*(_2bx*(0.5f - q2q2 - q3q3) + _2bz*(q1q3 - q0q2) - mx)
               + (-_2bx*q3 + _2bz*q1)*(_2bx*(q1q2 - q0q3) + _2bz*(q0q1 + q2q3) - my)
               + _2bx*q2*(_2bx*(q0q2 + q1q3) + _2bz*(0.5f - q1q1 - q2q2) - mz);
    float s1 =  2.0f*(q3*(2.0f*q1q3 - 2.0f*q0q2 - ax) + q0*(2.0f*q0q1 + 2.0f*q2q3 - ay))
               + _2bz*q3*(_2bx*(0.5f - q2q2 - q3q3) + _2bz*(q1q3 - q0q2) - mx)
               + (_2bx*q2 + _2bz*q0)*(_2bx*(q1q2 - q0q3) + _2bz*(q0q1 + q2q3) - my)
               + (_2bx*q3 - _4bz*q1)*(_2bx*(q0q2 + q1q3) + _2bz*(0.5f - q1q1 - q2q2) - mz);
    float s2 = -2.0f*(q0*(2.0f*q1q3 - 2.0f*q0q2 - ax) - q3*(2.0f*q0q1 + 2.0f*q2q3 - ay))
               + (-_4bx*q2 - _2bz*q0)*(_2bx*(0.5f - q2q2 - q3q3) + _2bz*(q1q3 - q0q2) - mx)
               + (_2bx*q1 + _2bz*q3)*(_2bx*(q1q2 - q0q3) + _2bz*(q0q1 + q2q3) - my)
               + (_2bx*q0 - _4bz*q2)*(_2bx*(q0q2 + q1q3) + _2bz*(0.5f - q1q1 - q2q2) - mz);
    float s3 =  2.0f*(q1*(2.0f*q1q3 - 2.0f*q0q2 - ax) + q2*(2.0f*q0q1 + 2.0f*q2q3 - ay))
               + (-_2bx*q2 + _2bz*q0)*(_2bx*(0.5f - q2q2 - q3q3) + _2bz*(q1q3 - q0q2) - mx)
               + (-_2bx*q1 + _2bz*q3)*(_2bx*(q1q2 - q0q3) + _2bz*(q0q1 + q2q3) - my)
               + _2bx*q2*(_2bx*(q0q2 + q1q3) + _2bz*(0.5f - q1q1 - q2q2) - mz);
    norm = 1.0f / sqrtf(s0*s0 + s1*s1 + s2*s2 + s3*s3);
    s0 *= norm; s1 *= norm; s2 *= norm; s3 *= norm;

    float qDot0 = 0.5f * (-q1*gx - q2*gy - q3*gz) - beta*s0;
    float qDot1 = 0.5f * ( q0*gx + q2*gz - q3*gy) - beta*s1;
    float qDot2 = 0.5f * ( q0*gy - q1*gz + q3*gx) - beta*s2;
    float qDot3 = 0.5f * ( q0*gz + q1*gy - q2*gx) - beta*s3;

    q0 += qDot0 * dt;
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;
    norm = 1.0f / sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    q0 *= norm; q1 *= norm; q2 *= norm; q3 *= norm;
  }
};
