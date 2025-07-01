# Call the Premake executable
& "VulkanCore\vendor\premake\premake5.exe" vs2022

# Check if the Assimp solution exists
if (Test-Path "VulkanCore\vendor\assimp\build\Assimp.sln") {
    Write-Host "Assimp is already built!" -ForegroundColor Green
} else {
    # Ask user whether to build Assimp
    $input = Read-Host "BUILD ASSIMP SOLUTION (Y/N)"

    if ($input -eq 'Y' -or $input -eq 'y') {
        Write-Host "Building Solution(.sln)" -ForegroundColor Green
        Set-Location "VulkanCore\vendor\assimp"
        & cmake -S . -B build -DASSIMP_BUILD_ZLIB=ON

        Write-Host "Building Debug Config" -ForegroundColor Cyan
        & cmake --build build --config Debug --target assimp

        Write-Host "Building Release Config" -ForegroundColor Cyan
        & cmake --build build --config Release --target assimp

        Set-Location ..\..\..
    }
}
