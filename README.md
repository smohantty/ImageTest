#Build Dependancy
 - This application depends on [rlottie](https://github.com/Samsung/rlottie) and [thorvg](https://github.com/Samsung/thorvg) library please make sure its installed in the system.
 
#Build
install [meson](http://mesonbuild.com/Getting-meson.html) and [ninja](https://ninja-build.org/) if not already installed.

Run meson to configure ImageTest
```
meson build
```
Run ninja to build ImageTest
```
ninja -C build


# Run
The imagetest application runs in 2 mode Test and Generation mode.

In Generation mode it finds all the resources in resource folder and generated baseline png files and store it in baseline folder.

Run In Generation mode
```
imagetest -g
```

In Test mode it finds all the resources in resource folder and renders using the library then it compares the result against
the baseline image stored in baseline folder. if both image are same the testcase is Passed otherwise Failed.

Run In Test mode
```
imagetest -t
```

If new resource needs to be added in the resource folder then ```imagetest -g``` should be run again to generate baseline images
for newly added resource. 
