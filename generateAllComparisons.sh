#!/bin/bash
set -e

THEIASFM_ROOT=$(dirname "$0")
SCRIPT_PATH="$THEIASFM_ROOT/generateComparisons.sh"

for DATASET_NAME in Roman_Forum Gendarmenmarkt Madrid_Metropolis Alamo
do
    for POSITION_ESTIMATOR in NONLINEAR LEAST_UNSQUARED_DEVIATION
    do
        for ROTATION_ESTIMATOR in NONLINEAR_QUATERNION_ROTATION_ERROR NONLINEAR
        do
            for ROBUST_LOSS_FUNCTION in TUKEY ARCTAN CAUCHY HUBER SOFTLONE NONE
            do
                for ROBUST_LOSS_WIDTH in 0.1 0.2 0.3
                do
                    for CONST_WEIGHT in false true
                    do
                        sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR" "$ROBUST_LOSS_FUNCTION" "$ROBUST_LOSS_WIDTH" "$CONST_WEIGHT"
                    done
                done
            done
        done

        ROTATION_ESTIMATOR=ROBUST_L1L2
        sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR"
    done
done

