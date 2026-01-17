# Symulacja fizyczna rozrywania tkanin w czasie rzeczywistym z wykorzystaniem modelu masa-sprężyna

## Installation
Works for fedora ;p

g++
sudo dnf install g++ 

opengl - GLEW, SDL2 (+SDL2_Image), GLM and FreeType
sudo dnf install glew-devel SDL2-devel SDL2_image-devel glm-devel freetype-devel

GLFW
sudo dnf install glfw-devel

download glad header file
https://glad.dav1d.de/
unzip glad.zip
sudo mv /path/glad/include/glad /usr/include
mv src file
mv /path/glad/src/glad.c /project_path/src

stb_image.h
https://github.com/nothings/stb
sudo mv /path/stb_image.h /usr/include/ 


## Building
mkdir build
cd build
cmake ..
make
./my_opengl_in_test