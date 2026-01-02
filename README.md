# CABEÇÃO TRACKING

For the interactive part of the project we wanted to track 3D position of the "big head" / Cabeção (We will call CB here) in space. the first go to idea is usually using a 6DOF sensor, but we wanted to avoid wires and batteries there, so we jumped for a passive solution.

## Camera
We decided to use a top camera to track some tag on CB
Tests began with the usual tools: OpenCV and ArUco markers. 
First tests were made with PS3Eye cam (great sensitivity, great frame rate, not enough resolution).
We find decent and inexpensive cameras on market with decent resolution and frame rate (based on the chosen chip: OmniVision OV9281)

## First Trackers
First CB prototypes and movements brought the first problem:
CB was suspended with two black elastic bands, so they occluded tracking image.
Researching ArUco alternatives with occlusion tolerance we started working with Stag and Coppertag. 
Stag was much easier to build and compile in C++ so the next prototype used this marker.

## ofWorks
in the meantime I was working on an openFrameworks fork to quickly deploy to multiple platforms, so it would help me deploying in Windows. I usually use macOS for development and using ofWorks allowed me to write everything on macOS and deploy to windows with no changes.
ofLibs also helped pinning specific OpenCV versions for all platforms minimizing differences.

## Tracking / Still not there
Testing STAG extensively showed it was still not ideal for our tracking system. We started to think of more simple solutions, inspired by motion capture systems (like optitrack).

## 2D Simple solution
After testing pose estimation we finally found the simplest solution possible. Tracking a circle in the camera.
With two different circles we could track CB angles and position. The way of individualizing circles was diameter ratios of 1:2.
Custom trackers were built made of big styrofoam spheres and reflective tape, so they were assumed aesthetically as CB "antennas"
This ratio of 1:2 would give resiliency to perspective distortion as we went for a wide camera (100 degrees)
Other improvement was using a camera with a IR pass filter and matching IR illumination, so most artifacts were filtered out.
On the top of 2D positioning and angle, we could detect Z axis by calculating the distance of the two spheres, having a callibration system to know when CB was in "Rest" position, or down or up.

## Communication with Unreal
All positioning data was sent to Unreal using OSC at 60hz locally. tracking system was installed in the same computer of visual content (Unreal) so only localhost OSC was needed.
