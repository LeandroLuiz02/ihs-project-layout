#!/bin/bash

echo "Installing dependencies for FPGA Pong game..."

# Update package list
sudo apt update

# Install SDL2 development libraries
sudo apt install -y libsdl2-dev libsdl2-ttf-dev

# Install build tools if not present
sudo apt install -y build-essential

echo "Dependencies installed successfully!"
echo ""
echo "To build and run the game:"
echo "1. Load your FPGA driver: sudo insmod driver/pci/de2i-150.ko"
echo "2. Create device file: sudo mknod /dev/de2i-150 c <major> 0"
echo "3. Set permissions: sudo chmod 666 /dev/de2i-150"
echo "4. Build game: make"
echo "5. Run game: ./target/release/app"
echo ""
echo "Game controls:"
echo "- SPACE: Start/Pause game"
echo "- W/S: Player 1 (left paddle)"
echo "- UP/DOWN: Player 2 (right paddle)"
echo "- R: Reset after game over"
