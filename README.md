# Custom Malloc Implementations

This project provides three different implementations of the `malloc` function in C++, each with varying levels of complexity and optimization. The implementations are named `malloc1`, `malloc2`, and `malloc3`. They offer increasing levels of sophistication and performance, making them suitable for different scenarios.

## Project Overview

Modern programming often requires dynamic memory allocation, and understanding how memory allocation works can greatly benefit software projects. This project delves into memory management by providing different implementations of the `malloc` function.

## Project Structure

### malloc1.cpp

`malloc1` is a simple and naive implementation of the `malloc` function. It provides basic memory allocation functionality, but lacks the sophistication of more advanced implementations.

### malloc2.cpp

`malloc2` builds upon the naive approach of `malloc1` by implementing a basic memory allocation algorithm that manages a linked list of memory blocks. This implementation aims to provide more efficient memory allocation compared to the naive version.

### malloc3.cpp

`malloc3` represents the culmination of improvements in memory allocation. It introduces advanced techniques like block splitting, merging, and efficient block ordering. This implementation is suitable for scenarios where memory efficiency and fragmentation reduction are critical.

## Installation

Follow these steps to set up and compile the project:

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/your-project.git
   cd your-project
   ```

2. Compile the project:
   ```bash
   make
   ```
   
## Contributing

This is an academic project and contributions will not be accepted. This repository is for reference and learning purposes only.
