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

