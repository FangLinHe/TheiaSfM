# Exploring Cost and Loss Functions in global SfM pipelines

The aim of our project is to explore cost and loss functions in global SfM pipelines.
Our code is based on [Theia library](http://theia-sfm.org/). You can check the
[original README.md](original_README.md) for more details.

A list of our modifications to the original code can be found in `diff_files.txt`.

## Setup the build environment

As Theia depends on a lot of libraries, e.g., OpenImageIO, RocksDB, RapidJSON, etc.,
setting up the build environment is quite frustrating. Thus, we have written a Dockerfile
to set up the build environment. Manually installing all the dependencies is also possible,
which we also made it compile successfully on MacBook Pro with M1 chip (ARM64), but
building dependent libraries from source with specific versions might be necessary.

## Download the dataset

We ran our experiments on [1DSfM dataset](https://www.cs.cornell.edu/projects/1dsfm/), where
you can download `Datasets (tar.gz, 642 MB)` without photos and download images under each
`Xyz images` link below. After downloading and uncompressing it, you can then move the
photos under the dataset name `datasets/Xyz/images`.

As mentioned in our report, we also reconstructed Grossmünster in Zürich using photos
collected from Flickr. You can download the photos [here](https://studentethzch-my.sharepoint.com/:f:/g/personal/fanghe_student_ethz_ch/EhekYKY78F1HqZHkymyTpikBuGjaIPRZ4jGw-cx_vz3QyA?e=yFvCiK)
or refer to the Jupyter-notebook `scripts/DownloadFlickrImages.ipynb` to see how to download
other dataset by yourself.

## Build the library and applications

Similar to other CMake projects, to build the library and applications, simply run:
```
mkdir build
cd build
cmake ..
make -j12
```

## View example reconstruction

We uploaded an example reconstruction of Grossmünster `data/Grossmünster-reconstruction-0`. Not sure
if it actually works to view reconstruction on a different machine, but you may try it:

```
cd build
./bin/view_reconstruction --reconstruction ../data/Grossmünster-reconstruction-0
```

## Generate all comparisons

In order to generate all comparisons of different parameters, e.g., rotation cost function,
position loss function, robustness width of rotation loss function, etc., on each 1DSfM dataset,
we have written a script `generateAllComparisons.sh` to run all the settings. Reconstruction
usually takes between 1 and 5 minutes, so for all combinations, it would take several hours if
not running in parallel.

After running comparisons, you can find the raw outputs of running `build/bin/compare_reconstructions`
under `logs/compare_reconstruction_results`. Then you can run `scripts/convertLogsToCsvs.py` to convert
these files to a csv file `logs/compare_reconstruction_results.csv`, which puts the settings, camera
counts, median and mean rotation / position errors, etc. into different columns.

Note that the experiments are not deterministic due to `--robust_alignment_threshold 1.0` parameter
when running `compare_reconstructions`, as it matches the cameras better using RANSAC. Without this
parameter, the errors are much larger than the reported performance on
[Theia-SfM website](http://theia-sfm.org/performance.html).

## Build reconstruction using our own data

To build the reconstruction using our own data, refer to the flags file
`applications/build_reconstruction_flags_grossmuenster_full.txt`. You need to change the paths:
`--images`, `--calibration_file`, `--output_reconstruction`, and `--matching_working_directory` to your
local folders or files.

Then, build the reconstruction:

```
build/bin/build_reconstruction --flagfile applications/build_reconstruction_flags_grossmuenster_full.txt
```

