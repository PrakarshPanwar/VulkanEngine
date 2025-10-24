#!/bin/sh

echo -n "Enter Run Configuration: "
read RUN_CONFIG
cd EditorLayer # Goto EditorLayer Directory

if [ $RUN_CONFIG == "d" ]; then
	../cmake-build-debug/EditorLayer/EditorLayer
elif [ $RUN_CONFIG == "r" ]; then
	../cmake-build-release/EditorLayer/EditorLayer
elif [ $RUN_CONFIG == "s" ]; then
	../cmake-build-release/EditorLayer/EditorLayer scenes/PhysicsDemo.vkscn
elif [ $RUN_CONFIG == "g" ]; then
	gamescope -W 1920 -H 1105 -r 60 ../cmake-build-release/EditorLayer/EditorLayer scenes/PhysicsDemo.vkscn
else
	echo Invalid Run Configuration!
fi
