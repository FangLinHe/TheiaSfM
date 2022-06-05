#!/bin/bash

# Install timeout command to set timeout
set -e
if [[ $OSTYPE == 'darwin'* ]] && ! command -v timeout &> /dev/null
then
    brew install coreutils
    alias timeout=gtimeout
fi

# Create root folder for logs
THEIASFM_ROOT=$(dirname "$0")
LOGS_ROOT="$THEIASFM_ROOT"/logs
if [[ ! -d "$LOGS_ROOT" ]]
then
    mkdir "$LOGS_ROOT"
fi

# Set up paths
FLAGFILE=../applications/build_1dsfm_reconstruction_flags_experiments.txt
DATASET_ROOT=/Users/fang-linhe/Documents/fl/ETH/3DVision/Project/Data/datasets
COMPARE_RECONSTRUCTION_LOG_DIR="$LOGS_ROOT"/compare_reconstruction_results
BUILD_RECONSTRUCTION_LOG_DIR="$LOGS_ROOT"/build_reconstruction_logs
CONVERT_GT_RECONSTRUCTION_LOG_DIR="$LOGS_ROOT"/convert_gt_reconstruction_logs

DATASET_NAME="${1:-Madrid_Metropolis}"
ROTATION_ESTIMATOR="${2:-NONLINEAR}"
POSITION_ESTIMATOR="${3:-LEAST_UNSQUARED_DEVIATION}"
ROBUST_LOSS_FUNCTION="${4:-HUBER}"
ROT_ROBUST_LOSS_WIDTH="${5:-0.01}"
POS_ROBUST_LOSS_WIDTH="${6:-0.005}"
CONST_WEIGHT="${7:-true}"

echo "================================================================="
echo "Generate comparisons for the following settings:"
echo "* DATASET_NAME: $DATASET_NAME"
echo "* ROTATION_ESTIMATOR: $ROTATION_ESTIMATOR"
echo "* POSITION_ESTIMATOR: $POSITION_ESTIMATOR"
if [[ "$ROTATION_ESTIMATOR" != ROBUST_L1L2 || "$POSITION_ESTIMATOR" == NONLINEAR ]]
then
    echo "* ROBUST_LOSS_FUNCTION: $ROBUST_LOSS_FUNCTION"
    echo "* ROT_ROBUST_LOSS_WIDTH: $ROT_ROBUST_LOSS_WIDTH"
    if [[ "$POSITION_ESTIMATOR" == NONLINEAR ]]
    then
        echo "* POS_ROBUST_LOSS_WIDTH: $POS_ROBUST_LOSS_WIDTH"
    fi
    if [[ "$ROTATION_ESTIMATOR" != ROBUST_L1L2 ]]
    then
        echo "* CONST_WEIGHT: $CONST_WEIGHT"
    fi
fi
echo "================================================================="

VIEW_RECONSTRUCTION=false
FORCE_BUILD_RECONSTRUCTION=false

WEIGHT_TYPE="$([ "$CONST_WEIGHT" = true ] && echo "CONST-WEIGHT" || echo "DYNAMIC-WEIGHT")"
ROT_WEIGHT_ARG=$([ "$CONST_WEIGHT" = true ] && echo "--rotation_estimation_const_weight" || echo "--norotation_estimation_const_weight")
POS_WEIGHT_ARG=$([ "$CONST_WEIGHT" = true ] && echo "--position_estimation_const_weight" || echo "--noposition_estimation_const_weight")

OUTPUT_RECONSTRUCTION_NAME="$DATASET_NAME-$ROTATION_ESTIMATOR"
if [[ "$POSITION_ESTIMATOR" != LEAST_UNSQUARED_DEVIATION ]]
then
    OUTPUT_RECONSTRUCTION_NAME="$OUTPUT_RECONSTRUCTION_NAME-$POSITION_ESTIMATOR"
fi
if [[ "$ROTATION_ESTIMATOR" != ROBUST_L1L2 ]]
then
    OUTPUT_RECONSTRUCTION_NAME="$OUTPUT_RECONSTRUCTION_NAME-$WEIGHT_TYPE-$ROBUST_LOSS_FUNCTION-$ROT_ROBUST_LOSS_WIDTH"
fi
if [[ "$POSITION_ESTIMATOR" = NONLINEAR ]]
then
    OUTPUT_RECONSTRUCTION_NAME="$OUTPUT_RECONSTRUCTION_NAME-$POS_ROBUST_LOSS_WIDTH"
fi
GT_RECONSTRUCTION_NAME="${DATASET_NAME}_gt_reconstruction"


if [[ ! -d "$COMPARE_RECONSTRUCTION_LOG_DIR" ]]
then
    mkdir "$COMPARE_RECONSTRUCTION_LOG_DIR"
fi
if [[ ! -d "$BUILD_RECONSTRUCTION_LOG_DIR" ]]
then
    mkdir "$BUILD_RECONSTRUCTION_LOG_DIR"
fi
if [[ ! -d "$CONVERT_GT_RECONSTRUCTION_LOG_DIR" ]]
then
    mkdir "$CONVERT_GT_RECONSTRUCTION_LOG_DIR"
fi

if [[ "$FORCE_BUILD_RECONSTRUCTION" = true || ! -f "$DATASET_ROOT/$DATASET_NAME/$OUTPUT_RECONSTRUCTION_NAME-0" ]]
then
    n=0
    t=10
    t_step=5
    until [ "$n" -ge 5 ]
    do
        echo "Building reconstruction...; see logs in $BUILD_RECONSTRUCTION_LOG_DIR"
        timeout "$t"m ./bin/build_1dsfm_reconstruction \
            --flagfile "$FLAGFILE" \
            --1dsfm_dataset_directory "$DATASET_ROOT/$DATASET_NAME" \
            --output_reconstruction "$DATASET_ROOT/$DATASET_NAME/$OUTPUT_RECONSTRUCTION_NAME" \
            "$ROT_WEIGHT_ARG" \
            "$POS_WEIGHT_ARG" \
            --global_rotation_estimator "$ROTATION_ESTIMATOR" \
            --global_position_estimator "$POSITION_ESTIMATOR" \
            --rotation_estimation_robust_loss_function "$ROBUST_LOSS_FUNCTION" \
            --rotation_estimation_robust_loss_width "$ROT_ROBUST_LOSS_WIDTH" \
            --position_estimation_robust_loss_function "$ROBUST_LOSS_FUNCTION" \
            --position_estimation_robust_loss_width "$POS_ROBUST_LOSS_WIDTH" \
            --log_dir "$BUILD_RECONSTRUCTION_LOG_DIR" \
            --nologtostderr \
        && break
        n=$((n+1))
        t=$((t+t_step))
        echo "Didn't finish building reconstruction in $((t-t_step)) minutes. Retrying with timeout $t minutes..."
        sleep 3
    done
fi

if [[ ! -f "$DATASET_ROOT/$DATASET_NAME/$GT_RECONSTRUCTION_NAME" ]];
then
    echo "Converting ground truth reconstruction...; see logs in $BUILD_RECONSTRUCTION_LOG_DIR"
    ./bin/convert_bundle_file \
        --lists_file "$DATASET_ROOT/$DATASET_NAME/list.txt" \
        --bundle_file "$DATASET_ROOT/$DATASET_NAME/gt_bundle.out" \
        --output_reconstruction_file "$DATASET_ROOT/$DATASET_NAME/$GT_RECONSTRUCTION_NAME" \
        --images_directory "$DATASET_ROOT/$DATASET_NAME/images/" \
        --log_dir "$CONVERT_GT_RECONSTRUCTION_LOG_DIR"
fi

echo "Comparing reconstruction to ground truth...; see logs in $COMPARE_RECONSTRUCTION_LOG_DIR"
./bin/compare_reconstructions \
    --reference_reconstruction "$DATASET_ROOT/$DATASET_NAME/$GT_RECONSTRUCTION_NAME" \
    --reconstruction_to_align "$DATASET_ROOT/$DATASET_NAME/$OUTPUT_RECONSTRUCTION_NAME-0" \
    --robust_alignment_threshold 1.0 \
    --log_dir "$COMPARE_RECONSTRUCTION_LOG_DIR"


LOG_FILE="$(readlink -f "$COMPARE_RECONSTRUCTION_LOG_DIR"/compare_reconstructions.INFO)"
mv "$LOG_FILE" "$LOG_FILE"."$OUTPUT_RECONSTRUCTION_NAME".txt


if [[ "$VIEW_RECONSTRUCTION" = true ]]
then
    echo "Viewing reconstruction $DATASET_ROOT/$DATASET_NAME/$GT_RECONSTRUCTION_NAME"
    ./bin/view_reconstruction \
        --reconstruction "$DATASET_ROOT/$DATASET_NAME/$GT_RECONSTRUCTION_NAME"
fi
