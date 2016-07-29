#!/bin/bash
ffmpeg -framerate 5 -i f%d.png -c:v libx264 -pix_fmt yuv420p out.mp4
