# Call the Premake executable
& "VulkanCore\vendor\premake\premake5.exe" vs2022

# Check if the Assimp solution exists
if (Test-Path "VulkanCore\vendor\assimp\Assimp.sln") {
    Write-Output "Assimp is already built!"
} else {
    # Ask user whether to build Assimp
    $input = Read-Host "BUILD ASSIMP SOLUTION (Y/N)"

    if ($input -eq 'Y' -or $input -eq 'y') {
        Set-Location "VulkanCore\vendor\assimp"
        & cmake -DASSIMP_BUILD_ZLIB=ON CMakeLists.txt

        Set-Location ..\..\..
    }
}

Pause
