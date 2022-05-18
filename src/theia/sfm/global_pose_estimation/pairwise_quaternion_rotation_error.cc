#include "theia/sfm/global_pose_estimation/pairwise_quaternion_rotation_error.h"

#include <ceres/ceres.h>
#include <Eigen/Core>

namespace theia {

PairwiseQuaternionRotationError::PairwiseQuaternionRotationError(
    const Eigen::Vector3d& relative_rotation,
    double weight)
    : relative_rotation_(relative_rotation), weight_(weight) {}

ceres::CostFunction* PairwiseQuaternionRotationError::Create(
    const Eigen::Vector3d& relative_rotation,
    double weight) {
  return new ceres::AutoDiffCostFunction<PairwiseQuaternionRotationError, 3, 3,
                                         3>(
      new PairwiseQuaternionRotationError(relative_rotation, weight));
}

}  // namespace theia
