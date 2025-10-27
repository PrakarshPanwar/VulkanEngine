# Call the Premake executable
& "VulkanCore\vendor\premake\premake5.exe" vs2026

# Check if the Assimp solution exists
if (Test-Path "VulkanCore\vendor\assimp\build\Assimp.slnx") {
    Write-Host "Assimp is already built!" -ForegroundColor Green
} else {
    # Ask user whether to build Assimp
    $input = Read-Host "BUILD ASSIMP SOLUTION(Y/N)"

    if ($input -eq 'Y' -or $input -eq 'y') {
        Write-Host "Building Solution(.slnx)" -ForegroundColor Green
        Set-Location "VulkanCore\vendor\assimp"
        & cmake -S . -B build -DASSIMP_BUILD_ZLIB=ON

        Write-Host "Building Debug Config" -ForegroundColor Cyan
        & cmake --build build --config Debug --target assimp

        Write-Host "Building Release Config" -ForegroundColor Cyan
        & cmake --build build --config Release --target assimp

        Set-Location ..\..\..
    }
}

# Check if Tracy Profiler solution exists
if (Test-Path "VulkanCore\vendor\tracy\profiler\build\tracy-profiler.slnx") {
    Write-Host "Tracy Profiler is already built!" -ForegroundColor Green
} else {
    # Ask user whether to build Tracy Profiler
    $input = Read-Host "BUILD TRACY PROFILER SOLUTION(Y/N)"

    if ($input -eq 'Y' -or $input -eq 'y') {
        Write-Host "Building Solution(.slnx)" -ForegroundColor Green
        Set-Location "VulkanCore\vendor\tracy\profiler"
        & cmake -S . -B build

        Write-Host "Building Release Config" -ForegroundColor Cyan
        & cmake --build build --config Release --target tracy-profiler

        Set-Location ..\..\..\..
    }
}
