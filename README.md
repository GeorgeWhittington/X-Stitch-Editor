# X-Stitch-Editor

A Cross Stitch pattern editor built with nanogui.

![screenshot of X-Stitch-Editor running](https://github.com/user-attachments/assets/69e03dfc-6125-4015-9dc0-0a253fc731d0)

## Running instructions

Install Cmake, and any build tools you need on your system to compile c++. Create a folder inside the root directory called build, and cd into it. Run the commands:

`cmake .. -DPDFHUMMUS_NO_DCT=TRUE -DPDFHUMMUS_NO_TIFF=TRUE`

`make`

NOTE: This code is only confirmed to be working on an intel Macbook, a M1 macbook and a Debian Linux computer. It may not function on a Windows PC.
