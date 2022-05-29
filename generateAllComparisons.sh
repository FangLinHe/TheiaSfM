#!/bin/bash
set -e

THEIASFM_ROOT=$(dirname "$0")
SCRIPT_PATH="$THEIASFM_ROOT/generateComparisons.sh"

for DATASET_NAME in Roman_Forum Madrid_Metropolis Alamo Gendarmenmarkt
do
    # for POSITION_ESTIMATOR in NONLINEAR
    # do
    #     for ROTATION_ESTIMATOR in NONLINEAR NONLINEAR_QUATERNION_ROTATION_ERROR
    #     do
    #         for ROBUST_LOSS_FUNCTION in TUKEY ARCTAN CAUCHY HUBER SOFTLONE
    #         do
    #             for ROT_ROBUST_LOSS_WIDTH in 0.05 0.1 0.2
    #             do
    #                 for POS_ROBUST_LOSS_WIDTH in 0.05 0.1 0.2
    #                 do
    #                     for CONST_WEIGHT in false true
    #                     do
    #                         sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR" "$ROBUST_LOSS_FUNCTION" "$ROT_ROBUST_LOSS_WIDTH" "$POS_ROBUST_LOSS_WIDTH" "$CONST_WEIGHT"
    #                     done
    #                 done
    #             done
    #         done
    #     done

    #     ROTATION_ESTIMATOR=ROBUST_L1L2
    #     for POS_ROBUST_LOSS_WIDTH in 0.05 0.1 0.2
    #     do
    #         for CONST_WEIGHT in false true
    #         do
    #             sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR" "$ROBUST_LOSS_FUNCTION" "$ROT_ROBUST_LOSS_WIDTH" "$POS_ROBUST_LOSS_WIDTH" "$CONST_WEIGHT"
    #         done
    #     done
    # done
    
    POSITION_ESTIMATOR=LEAST_UNSQUARED_DEVIATION
    for ROTATION_ESTIMATOR in NONLINEAR_QUATERNION_ROTATION_ERROR NONLINEAR ROBUST_L1L2
    do
        for ROBUST_LOSS_FUNCTION in TUKEY ARCTAN CAUCHY HUBER SOFTLONE
        do
            for ROT_ROBUST_LOSS_WIDTH in 0.05 0.1 0.2
            do
                for CONST_WEIGHT in false true
                do
                    sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR" "$ROBUST_LOSS_FUNCTION" "$ROT_ROBUST_LOSS_WIDTH" "$POS_ROBUST_LOSS_WIDTH" "$CONST_WEIGHT"
                done
            done
        done

        ROTATION_ESTIMATOR=ROBUST_L1L2
        sh "$SCRIPT_PATH" "$DATASET_NAME" "$ROTATION_ESTIMATOR" "$POSITION_ESTIMATOR"
    done
done

