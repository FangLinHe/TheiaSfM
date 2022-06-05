# Set up environment to build Theia-SfM

I managed to build the theia-sfm code in docker container. I pushed my changes to my GitHub in branch `os_unix`. I also wrote a Dockerfile, so what I did is:

```
cd /path/to/theia-sfm
git checkout os_unix
docker build -t myname/cpp-dev .
docker run --name build-theia-sfm -t -i -v /path/to/root/code:/usr/src myname/cpp-dev /bin/bash
```

Note that, I encountered one small issue when I compiled the code: the memory is not enough to compile the code, so I also increased the memory from 2G to 8G in docker.

Then, in the docker:
```
cd theia-sfm
mkdir build
cd build
cmake ..
make -j10
```

# Set up displaying in docker container and view in MacOS

## Set up Dockerfile

Already updated in Dockerfile, [[ref]](https://benit.github.io/blog/2019/02/15/ssh-x11-docker/).

## Set up MacOS

[[ref]](https://blog.mkari.de/posts/glx-on-mac/)

```
brew install --cask xquartz
brew install socat
```

After installing, reboot the machine. And check if DISPLAY variable is set:

```
echo "$DISPLAY"
# Prints: /private/tmp/com.apple.launchd.Wj854UinEm/org.xquartz:0 on my machine
```

## Run socat

```
socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\"
```

If it shows `2022/03/12 11:41:33 socat[3908] E bind(5, {LEN=0 AF=2 0.0.0.0:6000}, 16): Address already in use`, kill the processes:

```
lsof -n -i | grep 6000
# It shows the following processes:
# X11.bin    509 fanglin   11u  IPv6 0xcb4f295b1c3c89c5      0t0  TCP *:6000 (LISTEN)
# X11.bin    509 fanglin   12u  IPv4 0xcb4f29564f237dfd      0t0  TCP *:6000 (LISTEN)
kill -9 509
lsof -n -i | grep 6000
# This time nothing is printed
```

Then run socat CLI again. If no message, it'll hang there.

## Get IP address

Run `ifconfig en0` and get the address after `inet`, e.g.
```
	inet 192.168.1.10 netmask 0xffffff00 broadcast 192.168.1.255
```

We'll then use IP address `192.168.1.10`.

## Test with a simple docker container

```
docker run -e DISPLAY=192.168.1.10:0 gns3/xeyes
```

If everything is set up correctly, you should see X11 window showing rolling eyes.

## Run docker conatiner

```
docker run --name build-theia-sfm -t -i -v /Users/fanglin/Documents/.fl/ETH/3DVision/Project/Code:/usr/src -e DISPLAY=192.168.1.10:0 fl/cpp-dev /bin/bash
```

# Download the 1fSfM dataset and run applications

## Download dataset

Download [the dataset without images](http://landmark.cs.cornell.edu/projects/1dsfm/datasets.tar.gz) and extract the folders under `home/wilsonkl/projects/SfM_Init/datasets/* to `theia-sfm/datasets` (or whatever you like).

Then download images of corresponding dataset from [here](http://www.cs.cornell.edu/projects/1dsfm/) (under `Datasets (tar.gz, 642 MB)` are links to download images),
uncompress the file, and copy image folder to `theia-sfm/datasets/Something/images`.

## Run applications

Details can be found [here](http://theia-sfm.org/applications.html), but in short:
1. Copy `applications/build\_1dsfm\_reconstruction\_flags.txt` and modify the file. At least the input and output need to be set, e.g.,
   ```
   --1dsfm_dataset_directory=/usr/src/TheiaSfM/datasets/Gendarmenmarkt
   --output_reconstruction=/usr/src/TheiaSfM/results/Gendarmenmarkt
   ```
2. Run application: `./bin/build\_1dsfm\_reconstruction --flagfile=/path/to/build\_1dsfm\_reconstruction\_flags\_gendarmenmarkt.txt`
3. Visualize results: `./bin/view\_reconstruction --reconstruction=../results/Gendarmenmarkt-0`

