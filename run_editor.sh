#!/bin/sh

echo -n "Enter Run Configuration: "
read RUN_CONFIG
cd EditorLayer # Goto EditorLayer Directory

if [ $RUN_CONFIG == "d" ]; then
	XCURSOR_THEME=Breeze_Light XCURSOR_SIZE=24 ../cmake-build-debug/EditorLayer/EditorLayer
elif [ $RUN_CONFIG == "r" ]; then
	XCURSOR_THEME=Breeze_Light XCURSOR_SIZE=24 ../cmake-build-release/EditorLayer/EditorLayer
elif [ $RUN_CONFIG == "s" ]; then
	XCURSOR_THEME=Breeze_Light XCURSOR_SIZE=24 ../cmake-build-release/EditorLayer/EditorLayer scenes/PhysicsDemo.vkscn
elif [ $RUN_CONFIG == "g" ]; then
	XCURSOR_THEME=Breeze_Light XCURSOR_SIZE=24 gamescope -W 1920 -H 1105 -r 60 ../cmake-build-release/EditorLayer/EditorLayer scenes/PhysicsDemo.vkscn
else
	echo Invalid Run Configuration!
fi
