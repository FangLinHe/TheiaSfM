// Copyright (C) 2014 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Chris Sweeney (cmsweeney@cs.ucsb.edu)

#ifndef THEIA_SFM_GLOBAL_POSE_ESTIMATION_NONLINEAR_ROTATION_ESTIMATOR_H_
#define THEIA_SFM_GLOBAL_POSE_ESTIMATION_NONLINEAR_ROTATION_ESTIMATOR_H_

#include <Eigen/Core>
#include <ceres/ceres.h>
#include <unordered_map>

#include "theia/sfm/global_pose_estimation/pairwise_rotation_error.h"
#include "theia/sfm/global_pose_estimation/rotation_estimator.h"
#include "theia/sfm/types.h"
#include "theia/util/hash.h"

namespace theia {

// Computes the global rotations given relative rotations and an initial guess
// for the global orientations. Nonlinear optimization is performed with Ceres
// using a SoftL1 loss function to be robust to outliers.
//
// CostFunctionGenerator is a class which defines a function `Create` that
// generates a ceres::CostFunction* given a relative rotation in vector 3D:
// ceres::CostFunction* Create(const Eigen::Vector3d& relative_rotation)
// Check PairwiseRotationError as an example.

struct NonlinearRotationEstimatorOptions {
  // Options for Ceres nonlinear solver.
  LossFunctionType loss_function_type = LossFunctionType::HUBER;
  double robust_loss_width = 0.1;
  bool const_weight = false;  // set to const weight 1.0
  double min_weight = 0.5;
  int min_num_inlier_matches = 30;   // map to weight close to min_weight
  int max_num_inlier_matches = 200;  // map to weight close to 1.0
};

template <class CostFunctionGenerator = PairwiseRotationError>
class NonlinearRotationEstimator : public RotationEstimator {
public:
  using Options = NonlinearRotationEstimatorOptions;
  NonlinearRotationEstimator() : NonlinearRotationEstimator(0.1) {}

  explicit NonlinearRotationEstimator(const Options &options)
      : NonlinearRotationEstimator(
            options.loss_function_type, options.robust_loss_width,
            options.const_weight, options.min_weight,
            static_cast<double>(options.min_num_inlier_matches +
                                options.max_num_inlier_matches) *
                0.5,
            static_cast<double>(options.max_num_inlier_matches -
                                options.min_num_inlier_matches) /
                12.0) {}

  explicit NonlinearRotationEstimator(double robust_loss_width,
                                      bool const_weight = false,
                                      double min_weight = 0.5,
                                      int min_num_inlier_matches = 30,
                                      int max_num_inlier_matches = 200)
      : NonlinearRotationEstimator(LossFunctionType::SOFTLONE,
                                   robust_loss_width, const_weight, min_weight,
                                   static_cast<double>(min_num_inlier_matches +
                                                       max_num_inlier_matches) *
                                       0.5,
                                   static_cast<double>(max_num_inlier_matches -
                                                       min_num_inlier_matches) /
                                       12.0) {}

  // Estimates the global orientations of all views based on an initial
  // guess. Returns true on successful estimation and false otherwise.
  bool EstimateRotations(
      const std::unordered_map<ViewIdPair, TwoViewInfo> &view_pairs,
      std::unordered_map<ViewId, Eigen::Vector3d> *global_orientations) {
    CHECK_NOTNULL(global_orientations);
    if (global_orientations->size() == 0) {
      LOG(INFO) << "Skipping nonlinear rotation optimization because no "
                   "initialization was provivded.";
      return false;
    }
    if (view_pairs.size() == 0) {
      LOG(INFO) << "Skipping nonlinear rotation optimization because no "
                   "relative rotation constraints were provivded.";
      return false;
    }

    // Set up the problem and loss function.
    ceres::Problem::Options problem_options;
    problem_options.loss_function_ownership = ceres::DO_NOT_TAKE_OWNERSHIP;
    std::unique_ptr<ceres::Problem> problem(
        new ceres::Problem(problem_options));
    CHECK(loss_function_ != nullptr);

    for (const auto &view_pair : view_pairs) {
      const ViewIdPair &view_id_pair = view_pair.first;
      Eigen::Vector3d *rotation1 =
          FindOrNull(*global_orientations, view_id_pair.first);
      Eigen::Vector3d *rotation2 =
          FindOrNull(*global_orientations, view_id_pair.second);

      // Do not add the relative rotation constaint if it requires an
      // orientation that we do not have an initialization for.
      if (rotation1 == nullptr || rotation2 == nullptr) {
        continue;
      }

      ceres::CostFunction *cost_function = CostFunctionGenerator::Create(
          view_pair.second.rotation_2,
          (const_weight_) ? 1.0 : compute_weight(view_pair.second));
      problem->AddResidualBlock(cost_function, loss_function_.get(),
                                rotation1->data(), rotation2->data());
    }

    // The problem should be relatively sparse so sparse cholesky is a good
    // choice.
    ceres::Solver::Options options;
    options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
    options.max_num_iterations = 200;

    ceres::Solver::Summary summary;
    ceres::Solve(options, problem.get(), &summary);
    VLOG(1) << summary.FullReport();
    return true;
  }

private:
  NonlinearRotationEstimator(theia::LossFunctionType loss_function_type,
                             double robust_loss_width, bool const_weight,
                             double min_weight, double mid_point, double scale)
      : loss_function_(
            CreateLossFunction(loss_function_type, robust_loss_width)),
        const_weight_(const_weight), min_weight_(min_weight),
        mid_point_(mid_point), scale_(scale) {}

  double compute_weight(const TwoViewInfo& two_view_info) const {
    return compute_weight(two_view_info.num_verified_matches);
  }

  double compute_weight(int num_verified_matches) const {
    auto weight = 1 / (1 + exp(-(static_cast<double>(num_verified_matches) - mid_point_) / scale_));
    return min_weight_ + weight * (1.0 - min_weight_);
  }

  std::unique_ptr<ceres::LossFunction> loss_function_;
  bool const_weight_;
  double min_weight_;
  double mid_point_;
  double scale_;
};

} // namespace theia

#endif // THEIA_SFM_GLOBAL_POSE_ESTIMATION_NONLINEAR_ROTATION_ESTIMATOR_H_
