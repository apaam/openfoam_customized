## Get started

### Install

#### Prerequisites: 

The compilation requires ``gcc``, ``autoconf``, ``automake``, ``cmake``, ``mpi``, ``boost``, which can be obtained using

```
# For MacOS: use brew install, such as
brew install gcc autoconf automake cmake openmpi boost
        
# For Ubuntu: use apt-get install, such as
sudo apt-get install build-essential cmake
sudo apt-get install libopenmpi-dev openmpi-bin zlib1g-dev libboost-system-dev libboost-thread-dev 
sudo apt-get install flex bison gnuplot libreadline-dev libncurses-dev libxt-dev 
```

#### Compile and build:

```
make
``` 
