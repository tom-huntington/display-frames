# What does this do?

Reads raw rgb frames stdin and copies them to the swapchain.

# Build instructions

```
cmake -B build
cmake --build build
```

Binary will be located at `./build/samples/CopyBlitImage/CopyBlitImage`

# Prebuild Binary

I build a x86_64 linux binary for you

```
wget https://github.com/tom-huntington/display-frames/releases/latest/download/displayframes
```

# Command line Usage

you need to pass `--size {width}x{height}` as a command line argument

```
generate_frames() | displayframes --size 1280x720
```