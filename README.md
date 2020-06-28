# Non-Realtime Audio Processing
This project is a program for non-realtime processing of audio files. There are two parts to this project; the plugins; and the plugin host.

## Getting Started

### Prerequisites
* git
* A C++17 compiler

### Compilation
First clone the project into your current directory using:
```
git clone https://github.com/Dougal-s/Non-Realtime-Audio-Processing.git
cd Non-Realtime-Audio-Proccessing
```

#### Host:
```
cd host
mkdir build && cd build
cmake ..
make
```
The host binary can then be found in the host/build directory

#### Plugins:
After switching back to the root directory, run:
```
cd plugins
mkdir build && cd build
cmake ..
make
```
The plugin directories can then be found in the `plugins/build/Target Platform` directory.

**Note**: the plugin folders will be produced inside a folder of the same name e.g the plugin folder is Normalise/Normalise not Normalise.
