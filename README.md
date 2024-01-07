# X-Stitch-Editor

A Cross Stitch pattern editor built with nanogui. Currently using the python bindings for nanogui, but will likely be rewritten in C++ due to garbage collection issues with the nanogui bindings.

## Requirements

- [Python](https://www.python.org/downloads/) V3.11.3
- [Nanogui](https://pypi.org/project/nanogui/) V0.2.0

## Running instructions

Install python3, then create a virtual environment and activate it using these [instructions](https://docs.python.org/3/library/venv.html#creating-virtual-environments).

Install the requirements listed in requirements.txt using the command `python -m pip install -r requirements.txt`. Then use the command `python main.py` to run the application.

NOTE: I have only tested the functionality of the GPU kernels for MacOS, specifically on an M1 machine. This application may not yet work as intended on other devices.
