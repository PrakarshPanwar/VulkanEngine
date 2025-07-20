@echo off
IF EXIST VulkanCore\vendor\tracy\profiler\build\Release\tracy-profiler.exe (
	call VulkanCore\vendor\tracy\profiler\build\Release\tracy-profiler.exe
) ELSE (
	echo Tracy Profiler not found. Please build it first.
)
