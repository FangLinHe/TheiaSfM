import csv
from pathlib import Path
import re


LOG_FOLDER = Path(__file__).parent / "logs" / "compare_reconstruction_results"
CSV_OUTPUT = Path(__file__).parent / "logs" / "compare_reconstruction_results.csv"
DEDUPLICATE = True

if __name__ == '__main__':
    column_names = ['dataset', 'cost_function', 'weight_type', 'loss_function', 'robust_width',
                    'gt_total_cameras', 'total_cameras', 'common_cameras',
                    'gt_total_3d_points', 'total_3d_points',
                    'rotation_error_mean', 'rotation_error_median',
                    'position_error_mean', 'position_error_median',
                    'focal_length_error_mean', 'focal_length_error_median']
    keys = set()
    rows = []
    for log_path in LOG_FOLDER.glob('*.txt'):
        row = {}

        regexp1 = re.compile('compare_reconstructions\.\S+\.\S+\.\S+\.log\.\S+\.\S+\.\S+\.(\S+)-(\S+)-(\S+)-WEIGHT-(\S+)-(\S+)\.txt')
        regexp2 = re.compile('compare_reconstructions\.\S+\.\S+\.\S+\.log\.\S+\.\S+\.\S+\.(\S+)-(\S+)\.txt')
        re_match = regexp1.match(log_path.name)
        if re_match:
            dataset, cost_function, weight_type, loss_function, robust_width = re_match.groups()
        else:
            re_match = regexp2.match(log_path.name)
            assert re_match is not None
            dataset, cost_function = re_match.groups()
            weight_type = loss_function = robust_width = 'N/A'
        print(log_path)
        key = '-'.join((dataset, cost_function, weight_type, loss_function, robust_width))
        if key in keys:
            continue
        keys.add(key)
        row['dataset'] = dataset
        row['cost_function'] = cost_function
        row['weight_type'] = weight_type
        row['loss_function'] = loss_function
        row['robust_width'] = robust_width

        with open(log_path, 'rt') as log_file:
            line = log_file.readline().rstrip()
            while True:
                if line.endswith('Number of cameras:'):
                    gt_total_cameras_line = log_file.readline().strip()
                    gt_total_cameras = int(re.findall(r'\d+', gt_total_cameras_line)[-1])
                    row['gt_total_cameras'] = gt_total_cameras

                    total_cameras_line = log_file.readline().strip()
                    total_cameras = int(re.findall(r'\d+', total_cameras_line)[-1])
                    row['total_cameras'] = total_cameras

                    common_cameras_line = log_file.readline().strip()
                    common_cameras = int(re.findall(r'\d+', common_cameras_line)[-1])
                    row['common_cameras'] = common_cameras

                elif line.endswith('Number of 3d points:'):
                    gt_total_3d_points_line = log_file.readline().strip()
                    gt_total_3d_points = int(re.findall(r'\d+', gt_total_3d_points_line)[-1])
                    row['gt_total_3d_points'] = gt_total_3d_points

                    total_3d_points_line = log_file.readline().strip()
                    total_3d_points = int(re.findall(r'\d+', total_3d_points_line)[-1])
                    row['total_3d_points'] = total_3d_points
                elif line.endswith('Rotation Error:'):
                    rotation_error_mean_line = log_file.readline().strip()
                    rotation_error_mean = float(rotation_error_mean_line.split(' ')[-1])
                    row['rotation_error_mean'] = rotation_error_mean

                    rotation_error_median_line = log_file.readline().strip()
                    rotation_error_median = float(rotation_error_median_line.split(' ')[-1])
                    row['rotation_error_median'] = rotation_error_median
                elif line.endswith('Position Error:'):
                    position_error_mean_line = log_file.readline().strip()
                    position_error_mean = float(position_error_mean_line.split(' ')[-1])
                    row['position_error_mean'] = position_error_mean

                    position_error_median_line = log_file.readline().strip()
                    position_error_median = float(position_error_median_line.split(' ')[-1])
                    row['position_error_median'] = position_error_median
                elif line.endswith('Focal length errors:'):
                    focal_length_error_mean_line = log_file.readline().strip()
                    focal_length_error_mean = float(focal_length_error_mean_line.split(' ')[-1])
                    row['focal_length_error_mean'] = focal_length_error_mean

                    focal_length_error_median_line = log_file.readline().strip()
                    focal_length_error_median = float(focal_length_error_median_line.split(' ')[-1])
                    row['focal_length_error_median'] = focal_length_error_median

                    break

                line = log_file.readline().strip()

        assert len(set(row.keys()).difference(column_names)) == 0
        rows.append(row)
        
        with open(CSV_OUTPUT, 'wt') as output_file:
            dict_writer = csv.DictWriter(output_file, column_names)
            dict_writer.writeheader()
            dict_writer.writerows(rows)

            print(f"Processing done. CSV file: {CSV_OUTPUT}")
