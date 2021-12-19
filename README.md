# lajolla
UCSD CSE 272 renderer

# Build
All the dependencies are included. Use CMake to build.
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

# Acknowledgement
The renderer is heavily inspired by [pbrt](https://pbr-book.org/), [mitsuba](http://www.mitsuba-renderer.org/index_old.html), and [SmallVCM](http://www.smallvcm.com/).

We use [Embree](https://www.embree.org/) for ray casting.

We use [pugixml](https://pugixml.org/) to parse XML files.

We use [pcg](https://www.pcg-random.org/) for random number generation.

We use [stb_image](https://github.com/nothings/stb) and [tinyexr](https://github.com/syoyo/tinyexr) for reading & writing images.

We use [miniz](https://github.com/richgel999/miniz) for compression & decompression.
