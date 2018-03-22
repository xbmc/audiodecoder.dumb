# audiodecoder.dumb addon for Kodi

This is a [Kodi](http://kodi.tv) audio decoder addon for IT, XM and S3M files.

[![Build Status](https://travis-ci.org/notspiff/audiodecoder.dumb.svg?branch=master)](https://travis-ci.org/notspiff/audiodecoder.dumb)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/notspiff/audiodecoder.dumb?svg=true)](https://ci.appveyor.com/project/notspiff/audiodecoder-dumb)

## Build instructions

When building the addon you have to use the correct branch depending on which version of Kodi you're building against. 
For example, if you're building the `master` branch of Kodi you should checkout the `master` branch of this repository. 
Also make sure you follow this README from the branch in question.

### Linux

The following instructions assume you will have built Kodi already in the `kodi-build` directory 
suggested by the README.

1. `git clone https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/notspiff/audiodecoder.dumb.git`
3. `cd audiodecoder.dumb && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=audiodecoder.dumb -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/kodi-build/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

The addon files will be placed in `../../xbmc/kodi-build/addons` so if you build Kodi from source and run it directly 
the addon will be available as a system addon.