// Copyright (C) 2015 The Regents of the University of California (Regents).
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

#include <chrono>  // NOLINT
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string>
#include <theia/theia.h>
#include <time.h>
#include <vector>

#include "applications/command_line_helpers.h"

// Input/output files.
DEFINE_int32(max_num_images, 10000, "Maximum number of images to process.");
DEFINE_string(images, "", "Wildcard of images to reconstruct.");
DEFINE_string(image_masks, "", "Wildcard of image masks to reconstruct.");
DEFINE_string(matches_file, "", "Filename of the matches file.");
DEFINE_string(calibration_file,
              "",
              "Calibration file containing image calibration data.");
DEFINE_string(
    output_reconstruction,
    "",
    "Filename to write reconstruction to. The filename will be appended with "
    "the reconstruction number if multiple reconstructions are created.");

// Multithreading.
DEFINE_int32(num_threads,
             1,
             "Number of threads to use for feature extraction and matching.");

// Feature and matching options.
DEFINE_string(
    descriptor,
    "SIFT",
    "Type of feature descriptor to use. Must be one of the following: "
    "SIFT");
DEFINE_string(feature_density,
              "NORMAL",
              "Set to SPARSE, NORMAL, or DENSE to extract fewer or more "
              "features from each image.");
DEFINE_string(matching_strategy,
              "CASCADE_HASHING",
              "Strategy used to match features. Must be BRUTE_FORCE "
              " or CASCADE_HASHING");
DEFINE_string(matching_working_directory,
              "",
              "Directory used during matching to store features for "
              "out-of-core matching.");
DEFINE_double(lowes_ratio, 0.8, "Lowes ratio used for feature matching.");
DEFINE_double(max_sampson_error_for_verified_match,
              4.0,
              "Maximum sampson error for a match to be considered "
              "geometrically valid. This threshold is relative to an image "
              "with a width of 1024 pixels and will be appropriately scaled "
              "for images with different resolutions.");
DEFINE_int32(min_num_inliers_for_valid_match,
             30,
             "Minimum number of geometrically verified inliers that a pair on "
             "images must have in order to be considered a valid two-view "
             "match.");
DEFINE_bool(bundle_adjust_two_view_geometry,
            true,
            "Set to false to turn off 2-view BA.");
DEFINE_bool(keep_only_symmetric_matches,
            true,
            "Performs two-way matching and keeps symmetric matches.");
DEFINE_bool(select_image_pairs_with_global_image_descriptor_matching,
            true,
            "Use global descriptors to speed up image matching.");
DEFINE_int32(num_nearest_neighbors_for_global_descriptor_matching,
             100,
             "Number of nearest neighbor images to use for full descriptor "
             "matching.");
DEFINE_int32(num_gmm_clusters_for_fisher_vector,
             16,
             "Number of clusters to use for the GMM with Fisher Vectors for "
             "global image descriptors.");
DEFINE_int32(max_num_features_for_fisher_vector_training,
             1000000,
             "Number of features to use to train the Fisher Vector kernel for "
             "global image descriptor extraction.");

// Reconstruction building options.
DEFINE_string(reconstruction_estimator,
              "GLOBAL",
              "Type of SfM reconstruction estimation to use.");
DEFINE_bool(reconstruct_largest_connected_component,
            false,
            "If set to true, only the single largest connected component is "
            "reconstructed. Otherwise, as many models as possible are "
            "estimated.");
DEFINE_bool(shared_calibration,
            false,
            "Set to true if all camera intrinsic parameters should be shared "
            "as a single set of intrinsics. This is useful, for instance, if "
            "all images in the reconstruction were taken with the same "
            "camera.");
DEFINE_bool(only_calibrated_views,
            false,
            "Set to true to only reconstruct the views where calibration is "
            "provided or can be extracted from EXIF");
DEFINE_int32(min_track_length, 2, "Minimum length of a track.");
DEFINE_int32(max_track_length, 50, "Maximum length of a track.");
DEFINE_string(intrinsics_to_optimize,
              "NONE",
              "Set to control which intrinsics parameters are optimized during "
              "bundle adjustment.");
DEFINE_double(max_reprojection_error_pixels,
              4.0,
              "Maximum reprojection error for a correspondence to be "
              "considered an inlier after bundle adjustment.");

// Global SfM options.
DEFINE_string(global_rotation_estimator,
              "ROBUST_L1L2",
              "Type of global rotation estimation to use for global SfM.");
DEFINE_string(global_position_estimator,
              "NONLINEAR",
              "Type of global position estimation to use for global SfM.");
DEFINE_bool(refine_relative_translations_after_rotation_estimation,
            true,
            "Refine the relative translation estimation after computing the "
            "absolute rotations. This can help improve the accuracy of the "
            "position estimation.");
DEFINE_double(post_rotation_filtering_degrees,
              5.0,
              "Max degrees difference in relative rotation and rotation "
              "estimates for rotation filtering.");
DEFINE_bool(extract_maximal_rigid_subgraph,
            false,
            "If true, only cameras that are well-conditioned for position "
            "estimation will be used for global position estimation.");
DEFINE_bool(filter_relative_translations_with_1dsfm,
            true,
            "Filter relative translation estimations with the 1DSfM algorithm "
            "to potentially remove outlier relativep oses for position "
            "estimation.");
DEFINE_bool(refine_camera_positions_and_points_after_position_estimation,
            true,
            "After estimating positions in Global SfM we can refine only "
            "camera positions and 3D point locations, holding camera "
            "intrinsics and rotations constant. This often improves the "
            "stability of bundle adjustment when the camera intrinsics are "
            "inaccurate.");
DEFINE_int32(num_retriangulation_iterations,
             1,
             "Number of times to retriangulate any unestimated tracks. Bundle "
             "adjustment is performed after retriangulation.");

// Nonlinear rotation estimation options.
DEFINE_string(rotation_estimation_robust_loss_function,
              "SOFTLONE",
              "By setting this to an option other than NONE, a robust loss "
              "function will be used during rotation estimation which can "
              "improve robustness to outliers. Options are NONE, HUBER, "
              "SOFTLONE, CAUCHY, ARCTAN, and TUKEY.");
DEFINE_double(rotation_estimation_robust_loss_width,
              0.1,
              "Robust loss width to use for rotation estimation.");
DEFINE_bool(rotation_estimation_const_weight,
            false,
            "Use constant weight = 1.0 for all view pairs when computing "
            "rotation residuals.");
DEFINE_double(rotation_estimation_min_weight,
              0.5,
              "Minimum value of rotation residual weight, so the weights are in "
              "range [rotation_estimation_min_weight, 1].");
DEFINE_int32(rotation_estimation_min_num_inlier_matches,
             30,
             "Map the number of inlier matches to a weight, where this value "
             "would be mapped to the weight close to rotation_estimation_min_weight.");
DEFINE_int32(rotation_estimation_max_num_inlier_matches,
             200,
             "Map the number of inlier matches to a weight, where this value "
             "would be mapped to the weight close to 1.");

// Nonlinear position estimation options.
DEFINE_int32(
    position_estimation_min_num_tracks_per_view,
    0,
    "Minimum number of point to camera constraints for position estimation.");
DEFINE_string(position_estimation_robust_loss_function,
              "HUBER",
              "By setting this to an option other than NONE, a robust loss "
              "function will be used during position estimation which can "
              "improve robustness to outliers. Options are NONE, HUBER, "
              "SOFTLONE, CAUCHY, ARCTAN, and TUKEY.");
DEFINE_double(position_estimation_robust_loss_width,
              0.1,
              "Robust loss width to use for position estimation.");
DEFINE_bool(position_estimation_const_weight,
            false,
            "Use constant weight = 1.0 for all view pairs when computing "
            "position residuals.");
DEFINE_double(position_estimation_min_weight,
              0.5,
              "Minimum value of position residual weight, so the weights are in "
              "range [position_estimation_min_weight, 1].");
DEFINE_int32(position_estimation_min_num_inlier_matches,
             30,
             "Map the number of inlier matches to a weight, where this value "
             "would be mapped to the weight close to position_estimation_min_weight.");
DEFINE_int32(position_estimation_max_num_inlier_matches,
             200,
             "Map the number of inlier matches to a weight, where this value "
             "would be mapped to the weight close to 1.");

// Incremental SfM options.
DEFINE_double(absolute_pose_reprojection_error_threshold,
              4.0,
              "The inlier threshold for absolute pose estimation. This "
              "threshold is relative to an image with a width of 1024 pixels "
              "and will be appropriately scaled based on the input image "
              "resolutions.");
DEFINE_int32(min_num_absolute_pose_inliers,
             30,
             "Minimum number of inliers in order for absolute pose estimation "
             "to be considered successful.");
DEFINE_double(full_bundle_adjustment_growth_percent,
              5.0,
              "Full BA is only triggered for incremental SfM when the "
              "reconstruction has growth by this percent since the last time "
              "full BA was used.");
DEFINE_int32(partial_bundle_adjustment_num_views,
             20,
             "When full BA is not being run, partial BA is executed on a "
             "constant number of views specified by this parameter.");

// Triangulation options.
DEFINE_double(min_triangulation_angle_degrees,
              4.0,
              "Minimum angle between views for triangulation.");
DEFINE_double(
    triangulation_reprojection_error_pixels,
    15.0,
    "Max allowable reprojection error on initial triangulation of points.");
DEFINE_bool(bundle_adjust_tracks,
            true,
            "Set to true to optimize tracks immediately upon estimation.");

// Bundle adjustment parameters.
DEFINE_string(bundle_adjustment_robust_loss_function,
              "NONE",
              "By setting this to an option other than NONE, a robust loss "
              "function will be used during bundle adjustment which can "
              "improve robustness to outliers. Options are NONE, HUBER, "
              "SOFTLONE, CAUCHY, ARCTAN, and TUKEY.");
DEFINE_double(bundle_adjustment_robust_loss_width,
              10.0,
              "If the BA loss function is not NONE, then this value controls "
              "where the robust loss begins with respect to reprojection error "
              "in pixels.");

// Track Subsampling parameters.
DEFINE_bool(subsample_tracks_for_bundle_adjustment,
            false,
            "Set to true to subsample tracks used for bundle adjustment. This "
            "can help improve efficiency of bundle adjustment dramatically "
            "when used properly.");
DEFINE_int32(track_subset_selection_long_track_length_threshold,
             10,
             "When track subsampling is enabled, longer tracks are chosen with "
             "a higher probability with the track length capped to this value "
             "for selection.");
DEFINE_int32(track_selection_image_grid_cell_size_pixels,
             100,
             "When track subsampling is enabled, tracks are chosen such that "
             "each view has a good spatial coverage. This is achieved by "
             "binning tracks into an image grid in each view and choosing the "
             "best tracks in each grid cell to guarantee spatial coverage. The "
             "image grid cells are defined to be this width in pixels.");
DEFINE_int32(min_num_optimized_tracks_per_view,
             100,
             "When track subsampling is enabled, tracks are selected such that "
             "each view observes a minimum number of optimized tracks.");

using theia::FeaturesAndMatchesDatabase;
using theia::Reconstruction;
using theia::ReconstructionBuilder;
using theia::ReconstructionBuilderOptions;

// Sets the feature extraction, matching, and reconstruction options based on
// the command line flags. There are many more options beside just these located
// in //theia/vision/sfm/reconstruction_builder.h
ReconstructionBuilderOptions SetReconstructionBuilderOptions() {
  ReconstructionBuilderOptions options;
  options.num_threads = FLAGS_num_threads;

  options.descriptor_type = StringToDescriptorExtractorType(FLAGS_descriptor);
  options.feature_density = StringToFeatureDensity(FLAGS_feature_density);
  options.features_and_matches_database_directory =
      FLAGS_matching_working_directory;
  options.matching_strategy =
      StringToMatchingStrategyType(FLAGS_matching_strategy);
  options.matching_options.lowes_ratio = FLAGS_lowes_ratio;
  options.matching_options.keep_only_symmetric_matches =
      FLAGS_keep_only_symmetric_matches;
  options.min_num_inlier_matches = FLAGS_min_num_inliers_for_valid_match;
  options.matching_options.perform_geometric_verification = true;
  options.matching_options.geometric_verification_options
      .estimate_twoview_info_options.max_sampson_error_pixels =
      FLAGS_max_sampson_error_for_verified_match;
  options.matching_options.geometric_verification_options.bundle_adjustment =
      FLAGS_bundle_adjust_two_view_geometry;
  options.matching_options.geometric_verification_options
      .triangulation_max_reprojection_error =
      FLAGS_triangulation_reprojection_error_pixels;
  options.matching_options.geometric_verification_options
      .min_triangulation_angle_degrees = FLAGS_min_triangulation_angle_degrees;
  options.matching_options.geometric_verification_options
      .final_max_reprojection_error = FLAGS_max_reprojection_error_pixels;
  options.select_image_pairs_with_global_image_descriptor_matching =
      FLAGS_select_image_pairs_with_global_image_descriptor_matching;
  options.num_nearest_neighbors_for_global_descriptor_matching =
      FLAGS_num_nearest_neighbors_for_global_descriptor_matching;
  options.num_gmm_clusters_for_fisher_vector =
      FLAGS_num_gmm_clusters_for_fisher_vector;
  options.max_num_features_for_fisher_vector_training =
      FLAGS_max_num_features_for_fisher_vector_training;

  options.min_track_length = FLAGS_min_track_length;
  options.max_track_length = FLAGS_max_track_length;

  // Reconstruction Estimator Options.
  theia::ReconstructionEstimatorOptions& reconstruction_estimator_options =
      options.reconstruction_estimator_options;
  reconstruction_estimator_options.min_num_two_view_inliers =
      FLAGS_min_num_inliers_for_valid_match;
  reconstruction_estimator_options.num_threads = FLAGS_num_threads;
  reconstruction_estimator_options.intrinsics_to_optimize =
      StringToOptimizeIntrinsicsType(FLAGS_intrinsics_to_optimize);
  options.reconstruct_largest_connected_component =
      FLAGS_reconstruct_largest_connected_component;
  options.only_calibrated_views = FLAGS_only_calibrated_views;
  reconstruction_estimator_options.max_reprojection_error_in_pixels =
      FLAGS_max_reprojection_error_pixels;

  // Which type of SfM pipeline to use (e.g., incremental, global, etc.);
  reconstruction_estimator_options.reconstruction_estimator_type =
      StringToReconstructionEstimatorType(FLAGS_reconstruction_estimator);

  // Global SfM Options.
  reconstruction_estimator_options.global_rotation_estimator_type =
      StringToRotationEstimatorType(FLAGS_global_rotation_estimator);
  reconstruction_estimator_options.global_position_estimator_type =
      StringToPositionEstimatorType(FLAGS_global_position_estimator);
  reconstruction_estimator_options.num_retriangulation_iterations =
      FLAGS_num_retriangulation_iterations;
  reconstruction_estimator_options
      .refine_relative_translations_after_rotation_estimation =
      FLAGS_refine_relative_translations_after_rotation_estimation;
  reconstruction_estimator_options.extract_maximal_rigid_subgraph =
      FLAGS_extract_maximal_rigid_subgraph;
  reconstruction_estimator_options.filter_relative_translations_with_1dsfm =
      FLAGS_filter_relative_translations_with_1dsfm;

  reconstruction_estimator_options.rotation_filtering_max_difference_degrees =
      FLAGS_post_rotation_filtering_degrees;
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .loss_function_type =
      StringToLossFunction(FLAGS_rotation_estimation_robust_loss_function);
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .robust_loss_width = FLAGS_rotation_estimation_robust_loss_width;
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .const_weight = FLAGS_rotation_estimation_const_weight;
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .min_weight = FLAGS_rotation_estimation_min_weight;
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .min_num_inlier_matches =
      FLAGS_rotation_estimation_min_num_inlier_matches;
  reconstruction_estimator_options.nonlinear_rotation_estimator_options
      .max_num_inlier_matches =
      FLAGS_rotation_estimation_max_num_inlier_matches;

  reconstruction_estimator_options.nonlinear_position_estimator_options
      .min_num_points_per_view =
      FLAGS_position_estimation_min_num_tracks_per_view;
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .loss_function_type =
      StringToLossFunction(FLAGS_position_estimation_robust_loss_function);
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .robust_loss_width = FLAGS_position_estimation_robust_loss_width;
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .const_weight = FLAGS_position_estimation_const_weight;
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .min_weight = FLAGS_position_estimation_min_weight;
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .min_num_inlier_matches =
      FLAGS_position_estimation_min_num_inlier_matches;
  reconstruction_estimator_options.nonlinear_position_estimator_options
      .max_num_inlier_matches =
      FLAGS_position_estimation_max_num_inlier_matches;
  reconstruction_estimator_options
      .refine_camera_positions_and_points_after_position_estimation =
      FLAGS_refine_camera_positions_and_points_after_position_estimation;

  // Incremental SfM Options.
  reconstruction_estimator_options.absolute_pose_reprojection_error_threshold =
      FLAGS_absolute_pose_reprojection_error_threshold;
  reconstruction_estimator_options.min_num_absolute_pose_inliers =
      FLAGS_min_num_absolute_pose_inliers;
  reconstruction_estimator_options.full_bundle_adjustment_growth_percent =
      FLAGS_full_bundle_adjustment_growth_percent;
  reconstruction_estimator_options.partial_bundle_adjustment_num_views =
      FLAGS_partial_bundle_adjustment_num_views;

  // Triangulation options (used by all SfM pipelines).
  reconstruction_estimator_options.min_triangulation_angle_degrees =
      FLAGS_min_triangulation_angle_degrees;
  reconstruction_estimator_options
      .triangulation_max_reprojection_error_in_pixels =
      FLAGS_triangulation_reprojection_error_pixels;
  reconstruction_estimator_options.bundle_adjust_tracks =
      FLAGS_bundle_adjust_tracks;

  // Bundle adjustment options (used by all SfM pipelines).
  reconstruction_estimator_options.bundle_adjustment_loss_function_type =
      StringToLossFunction(FLAGS_bundle_adjustment_robust_loss_function);
  reconstruction_estimator_options.bundle_adjustment_robust_loss_width =
      FLAGS_bundle_adjustment_robust_loss_width;

  // Track subsampling options.
  reconstruction_estimator_options.subsample_tracks_for_bundle_adjustment =
      FLAGS_subsample_tracks_for_bundle_adjustment;
  reconstruction_estimator_options
      .track_subset_selection_long_track_length_threshold =
      FLAGS_track_subset_selection_long_track_length_threshold;
  reconstruction_estimator_options.track_selection_image_grid_cell_size_pixels =
      FLAGS_track_selection_image_grid_cell_size_pixels;
  reconstruction_estimator_options.min_num_optimized_tracks_per_view =
      FLAGS_min_num_optimized_tracks_per_view;
  return options;
}

void AddMatchesToReconstructionBuilder(
    FeaturesAndMatchesDatabase* features_and_matches_database,
    ReconstructionBuilder* reconstruction_builder) {
  // Add all the views. When the intrinsics group id is invalid, the
  // reconstruction builder will assume that the view does not share its
  // intrinsics with any other views.
  theia::CameraIntrinsicsGroupId intrinsics_group_id =
      theia::kInvalidCameraIntrinsicsGroupId;
  if (FLAGS_shared_calibration) {
    intrinsics_group_id = 0;
  }

  const auto camera_calibrations_names =
      features_and_matches_database->ImageNamesOfCameraIntrinsicsPriors();
  LOG(INFO) << "Loading " << camera_calibrations_names.size()
            << " intrinsics priors from the DB.";
  for (int i = 0; i < camera_calibrations_names.size(); i++) {
    const auto camera_intrinsics_prior =
        features_and_matches_database->GetCameraIntrinsicsPrior(
            camera_calibrations_names[i]);
    reconstruction_builder->AddImageWithCameraIntrinsicsPrior(
        camera_calibrations_names[i],
        camera_intrinsics_prior,
        intrinsics_group_id);
  }

  // Add the matches.
  const auto match_keys = features_and_matches_database->ImageNamesOfMatches();
  LOG(INFO) << "Loading " << match_keys.size() << " matches from the DB.";
  for (const auto& match_key : match_keys) {
    const theia::ImagePairMatch& match =
        features_and_matches_database->GetImagePairMatch(match_key.first,
                                                         match_key.second);
    CHECK(reconstruction_builder->AddTwoViewMatch(
        match_key.first, match_key.second, match));
  }
}

void AddImagesToReconstructionBuilder(
    ReconstructionBuilder* reconstruction_builder) {
  std::vector<std::string> image_files;
  CHECK(theia::GetFilepathsFromWildcard(FLAGS_images, &image_files))
      << "Could not find images that matched the filepath: " << FLAGS_images
      << ". NOTE that the ~ filepath is not supported.";

  CHECK_GT(image_files.size(), 0) << "No images found in: " << FLAGS_images;

  if (image_files.size() > FLAGS_max_num_images) {
    image_files.resize(FLAGS_max_num_images);
  }
  
  // Load calibration file if it is provided.
  std::unordered_map<std::string, theia::CameraIntrinsicsPrior>
      camera_intrinsics_prior;
  if (FLAGS_calibration_file.size() != 0) {
    CHECK(theia::ReadCalibration(FLAGS_calibration_file,
                                 &camera_intrinsics_prior))
        << "Could not read calibration file.";
  }

  // Add images with possible calibration. When the intrinsics group id is
  // invalid, the reconstruction builder will assume that the view does not
  // share its intrinsics with any other views.
  theia::CameraIntrinsicsGroupId intrinsics_group_id =
      theia::kInvalidCameraIntrinsicsGroupId;
  if (FLAGS_shared_calibration) {
    intrinsics_group_id = 0;
  }

  for (const std::string& image_file : image_files) {
    std::string image_filename;
    CHECK(theia::GetFilenameFromFilepath(image_file, true, &image_filename));

    const theia::CameraIntrinsicsPrior* image_camera_intrinsics_prior =
        FindOrNull(camera_intrinsics_prior, image_filename);
    if (image_camera_intrinsics_prior != nullptr) {
      CHECK(reconstruction_builder->AddImageWithCameraIntrinsicsPrior(
          image_file, *image_camera_intrinsics_prior, intrinsics_group_id));
    } else {
      CHECK(reconstruction_builder->AddImage(image_file, intrinsics_group_id));
    }
  }

  // Add black and write image masks for any images if those are provided.
  // The white part of the mask indicates the area for the keypoints extraction.
  // The mask is a basic black and white image (jpg, png, tif etc.), where white
  // is 1.0 and black is 0.0. Its name must content the associated image's name
  // (e.g. 'image0001_mask.jpg' is the mask of 'image0001.png').
  std::vector<std::string> mask_files;
  if (FLAGS_image_masks.size() != 0) {
    CHECK(theia::GetFilepathsFromWildcard(FLAGS_image_masks, &mask_files))
        << "Could not find image masks that matched the filepath: "
        << FLAGS_image_masks << ". NOTE that the ~ filepath is not supported.";
    if (mask_files.size() > 0) {
      for (const std::string& image_file : image_files) {
        std::string image_filename;
        CHECK(
            theia::GetFilenameFromFilepath(image_file, false, &image_filename));
        // Find and add the associated mask
        for (const std::string& mask_file : mask_files) {
          if (mask_file.find(image_filename) != std::string::npos) {
            CHECK(reconstruction_builder->AddMaskForFeaturesExtraction(
                image_file, mask_file));
            break;
          }
        }
      }
    } else {
      LOG(WARNING) << "No image masks found in: " << FLAGS_image_masks;
    }
  }

  // Extract and match features.
  CHECK(reconstruction_builder->ExtractAndMatchFeatures());
}

int main(int argc, char* argv[]) {
  THEIA_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  CHECK_GT(FLAGS_output_reconstruction.size(), 0);

  // Initialize the features and matches database.
  std::unique_ptr<FeaturesAndMatchesDatabase> features_and_matches_database(
      new theia::RocksDbFeaturesAndMatchesDatabase(
          FLAGS_matching_working_directory));

  // Create the reconstruction builder.
  const ReconstructionBuilderOptions options =
      SetReconstructionBuilderOptions();
  ReconstructionBuilder reconstruction_builder(
      options, features_and_matches_database.get());

  // If matches are provided, load matches otherwise load images.
  if (features_and_matches_database->NumMatches() > 0) {
    AddMatchesToReconstructionBuilder(features_and_matches_database.get(),
                                      &reconstruction_builder);
  } else if (FLAGS_images.size() != 0) {
    AddImagesToReconstructionBuilder(&reconstruction_builder);
  } else {
    LOG(FATAL) << "You must specifiy either images to reconstruct or supply a "
                  "database with matches stored in it.";
  }

  std::vector<Reconstruction*> reconstructions;
  CHECK(reconstruction_builder.BuildReconstruction(&reconstructions))
      << "Could not create a reconstruction.";

  for (int i = 0; i < reconstructions.size(); i++) {
    const std::string output_file =
        theia::StringPrintf("%s-%d", FLAGS_output_reconstruction.c_str(), i);
    LOG(INFO) << "Writing reconstruction " << i << " to " << output_file;
    CHECK(theia::WriteReconstruction(*reconstructions[i], output_file))
        << "Could not write reconstruction to file.";
  }
}
