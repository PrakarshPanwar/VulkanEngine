@echo off
call VulkanCore\vendor\premake\premake5.exe vs2022
set /p input=BUILD ASSIMP SOLUTION(Y/N)=

IF %input%==Y (
cd VulkanCore\vendor\assimp
cmake -DASSIMP_BUILD_ZLIB=ON CMakeLists.txt
)

IF %input%==y (
cd VulkanCore\vendor\assimp
cmake -DASSIMP_BUILD_ZLIB=ON CMakeLists.txt
)

pause