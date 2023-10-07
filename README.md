# What does this do?

Reads raw rgb frames stdin and copies them to the swapchain.

Build instructions:
```
cmake -B build
cmake --build build
```

Binary will be located at `./build/samples/CopyBlitImage/CopyBlitImage`


you need to pass `--size {width}x{height}`
```
generate_frames() | displayframes --size 1280x720
```