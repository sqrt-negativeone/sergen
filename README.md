# SerGen
Meta program for generating c types (des)serializers, it uses **Metadesk** library

Genearte serializers/desrializers for your C types, the way you do it is by writing a metadesk ([website](https://dion.systems/metadesk.html)) file describing your types, and feed it to the meta program to generate the serializers/deserializers, an example is provided in in `example` folder

# Build
To build the metaprogram just run the `build.bat` on windows or `build.sh` on linux or macOS.

# Run
Run `.\build\sergen.exe [path to md file]` on windows or `./build/sergen [path to md file]` on linux or macos, make sure that a `generated` folder exist in the same folder where the metadesk file is, the serializers and deserializers will be generated there. 
