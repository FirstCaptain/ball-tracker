# Project summary

Simple script to watch a tennis ball on move across a blue court. 

### USAGE
~~~
./test inputvideo outputvideo
~~~

#### Dependencies / Prerequisites

opencv4.2
scons
docker optional but prefered

### Installation and execution instructions

##### Local Way

untested as i do this dev work on ubuntu 20.04 with docker

You need to install scons (for building the app)
~~~
sudo apt-get install scons
~~~

install opencv
helpful article here..
https://medium.com/@pokhrelsuruchi/setting-up-opencv-for-python-and-c-in-ubuntu-20-04-6b0331e37437

###### Prefered method

note: Modify the run.sh to point to the repo location on your machine
~~~
docker build -t docker-scons .
./run.sh
~~~

then run the usage requirements inside the docker container, if it's mapped to the volume will work as expected