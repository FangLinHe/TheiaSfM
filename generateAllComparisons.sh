#!/bin/bash
set -e

THEIASFM_ROOT=$(dirname "$0")
SCRIPT_PATH="$THEIASFM_ROOT/generateComparisons.sh"

for POSITION_ESTIMATOR in LEAST_UNSQUARED_DEVIATION
do
    for DATASET_NAME in Roman_Forum Gendarmenmarkt Madrid_Metropolis Alamo
    do
        for ROTATION_ESTIMATOR in NONLINEAR NONLINEAR_QUATERNION_ROTATION_ERROR
        do
            for ROBUST_LOSS_FUNCTION in NONE HUBER SOFTLONE CAUCHY ARCTAN TUKEY
            do
                for ROBUST_LOSS_WIDTH in 0.05 0.1 0.15 0.2 0.25 0.3 0.35
                do
                    for CONST_WEIGHT in true false
                    do
                        sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR" "$ROBUST_LOSS_FUNCTION" "$ROBUST_LOSS_WIDTH" "$CONST_WEIGHT"
                    done
                done
            done
        done
    done

    ROTATION_ESTIMATOR=ROBUST_L1L2
    sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR"
done

