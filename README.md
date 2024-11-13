# Compiling the project
```
git clone https://github.com/ItaloYt/PXCube.git pxcube
cd pxcube/
git clone https://github.com/ItaloYt/Phoenix-Engine.git phoenix
mkdir include/
ln -s phoenix/include/* include/
make
```
The makefile will call the phoenix engine makefile and will use the libraries generated.
This will produce an executable pxcube, which draws a simple triangle with vulkan.
