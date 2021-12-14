# lajolla
UCSD CSE 272 renderer

# Build
There is no dependency. Use CMake to build.
If you are on Unix systems, try
```
mkdir build
cd build
cmake ..
```

# Run
Try 
```
cd build
./lajolla ../scenes/cbox/cbox.xml
```
This will generate an image "image.pfm".

To view the image, use [hdrview](https://github.com/wkjarosz/hdrview), or [tev](https://github.com/Tom94/tev).
