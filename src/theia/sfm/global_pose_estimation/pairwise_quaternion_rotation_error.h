#ifndef THEIA_SFM_GLOBAL_POSE_ESTIMATION_PAIRWISE_QUATERNION_ROTATION_ERROR_H_
#define THEIA_SFM_GLOBAL_POSE_ESTIMATION_PAIRWISE_QUATERNION_ROTATION_ERROR_H_

// #include <iostream>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <ceres/rotation.h>

namespace ceres {
class CostFunction;
} // namespace ceres

namespace theia {

// The error in two global rotations based on the current estimates for the
// global rotations and the relative rotation such that
// d_quaternion(R_1, R_2) = min(|| q_1 + q_2 ||_2, || q1 - q2 ||_2)
// Ref: https://link.springer.com/content/pdf/10.1007/s11263-012-0601-0.pdf
// See Quaternion Distance on page 276

template <typename T> T quaternion_absolute(const Eigen::Quaternion<T> &q) {
  return sqrt(q.w() * q.w() + q.x() * q.x() + q.y() * q.y() + q.z() * q.z());
}

template <typename T>
Eigen::Quaternion<T> quaternion_subtract(const Eigen::Quaternion<T> &q1,
                                         const Eigen::Quaternion<T> &q2) {
  return Eigen::Quaternion<T>{q1.w() - q2.w(), q1.x() - q2.x(), q1.y() - q2.y(),
                              q1.z() - q2.z()};
}

struct PairwiseQuaternionRotationError {
  PairwiseQuaternionRotationError(const Eigen::Vector3d &relative_rotation,
                                  double weight = 1.0);

  // The error is given by the rotation loop error as specified above. We return
  // 3 residuals to give more opportunity for optimization.
  template <typename T>
  bool operator()(const T *rotation1, const T *rotation2, T *residuals) const;

  static ceres::CostFunction *Create(const Eigen::Vector3d &relative_rotation,
                                     double weight = 1.0);

private:
  const Eigen::Vector3d relative_rotation_;
  const double weight_;
};

template <typename T>
Eigen::Quaternion<T>
ceres_quaternion_to_eigen(const Eigen::Matrix<T, 4, 1> &ceres_quat) {
  return Eigen::Quaternion<T>{ceres_quat[0], ceres_quat[1], ceres_quat[2],
                              ceres_quat[3]};
}

template <typename T>
Eigen::Matrix<T, 4, 1>
eigen_quaternion_to_ceres(const Eigen::Quaternion<T> &eigen_quat) {
  return Eigen::Matrix<T, 4, 1>{eigen_quat.w(), eigen_quat.x(), eigen_quat.y(),
                                eigen_quat.z()};
}


/**
 * eps = 1e-15
 * q = q / (np.linalg.norm(q) + eps)
 * q_gt = q_gt / (np.linalg.norm(q_gt) + eps)
 * loss_q = np.maximum(eps, (1.0 - np.sum(q * q_gt)**2))
 * err_q = np.arccos(1 - 2 * loss_q)
 */
template <typename T>
bool PairwiseQuaternionRotationError::operator()(const T *rotation1,
                                                 const T *rotation2,
                                                 T *residuals) const {
  Eigen::Matrix<T, 4, 1> quaternion1, quaternion2;
  Eigen::Vector4d relative_rotation_quat;
  ceres::AngleAxisToQuaternion(rotation1, quaternion1.data());
  ceres::AngleAxisToQuaternion(rotation2, quaternion2.data());
  ceres::AngleAxisToQuaternion(relative_rotation_.data(),
                               relative_rotation_quat.data());

  // Compute the loop rotation from the two global rotations.
  const Eigen::Quaternion<T> loop_rotation_quat =
      (ceres_quaternion_to_eigen<T>(quaternion2) *
       ceres_quaternion_to_eigen<T>(quaternion1).inverse())
          .normalized();

  // loss_q = np.maximum(eps, (1.0 - np.sum(q * q_gt)**2))
  const double eps = 1e-15;
  const Eigen::Quaternion<T> gt_quat =
      ceres_quaternion_to_eigen<T>(relative_rotation_quat.cast<T>()).normalized();
  auto quaternion_mulsum = loop_rotation_quat.dot(gt_quat);
  auto loss_q = fmax(eps, 1.0 - quaternion_mulsum * quaternion_mulsum);
  auto err_q = acos(1.0 - 2.0 * loss_q);

  residuals[0] = weight_ * err_q;

  return true;
}

} // namespace theia

#endif // THEIA_SFM_GLOBAL_POSE_ESTIMATION_PAIRWISE_QUATERNION_ROTATION_ERROR_H_
