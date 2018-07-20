# gm_navigation

Fixed to work on linux, but broken for windows in the process. I'll make this properly multiplatform soon. 

Some quirks to be aware of:
- You'll notice that the CreateThreadPool and DestroyThreadPool functions are named V_CreateThreadPool and V_DestroyThreadPool. This is because the symbols in the garry's mod version of the vstdlib shared object differ from the function names in the sdk source. If you want to compile, fix them in in `source-sdk-20133/mp/src/public/vstdlib/jobthread.h`
- Any 4.2 to 4.9 version of gcc might work, maybe. For the public builds, I used the Steam Client Runtime available [HERE](https://developer.valvesoftware.com/wiki/Source_SDK_2013#Source_SDK_2013_on_Linux). Use that to be sure it'll compile. 

###  Dependencies:
- [source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013)
- [garrysmod_common](https://github.com/danielga/garrysmod_common)

Place the folders at the same directory level as this repository. See the premake file for details.  

### Building

```
$ export PATH=/path/to/valve/steam-runtime/bin:$PATH
$ cd gm_navigation
$ ./premake5 gmake
$ cd project
$ make
```

### Important!

Please don't call any navmesh functions before InitPostEntity, or you'll have a bad day! 

