@echo off
if not defined DevEnvDir (
    call vcvarsall x64
)

set IncludeDir="F:\Env\vcpkg\installed\x64-windows\include"
set LibDir="F:\Env\vcpkg\installed\x64-windows\lib"

set VulkanInclude="C:\VulkanSDK\1.2.189.2\Include"
set VulkanLib="C:\VulkanSDK\1.2.189.2\Lib"

set CompileFlags=-MTd -nologo -fp:fast -EHa -Od -WX- -W4 -Oi -GR- -Gm- -GS -wd4100 -wd4201 -D_DEBUG=1 -DRTX=0 -FC -Z7 -I %IncludeDir% -I %VulkanInclude%
set LinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console /LIBPATH:%LibDir% /LIBPATH:VulkanLib /NODEFAULTLIB:msvcrt.lib

REM Here is shaders building
if not exist ..\shaders mkdir ..\shaders
if exist triangle.vert.glsl call glslangValidator triangle.vert.glsl -V -o ..\shaders\triangle.vert.spv
if exist triangle.frag.glsl call glslangValidator triangle.frag.glsl -V -o ..\shaders\triangle.frag.spv
if exist meshlet.vert.glsl call glslangValidator meshlet.vert.glsl -V -o ..\shaders\meshlet.vert.spv
if exist meshfvf.vert.glsl call glslangValidator meshfvf.vert.glsl -V -o ..\shaders\meshfvf.vert.spv
if exist ..\shaders\triangle.vert.glsl call glslangValidator ..\shaders\triangle.vert.glsl -V -o ..\shaders\triangle.vert.spv
if exist ..\shaders\triangle.frag.glsl call glslangValidator ..\shaders\triangle.frag.glsl -V -o ..\shaders\triangle.frag.spv
if exist ..\shaders\meshlet.vert.glsl call glslangValidator ..\shaders\meshlet.vert.glsl -V -o ..\shaders\meshlet.vert.spv
if exist ..\shaders\meshfvf.vert.glsl call glslangValidator ..\shaders\meshfvf.vert.glsl -V -o ..\shaders\meshfvf.vert.spv
xcopy triangle.vert.glsl ..\shaders\
xcopy triangle.frag.glsl ..\shaders\
xcopy meshlet.vert.glsl ..\shaders\
xcopy meshfvf.vert.glsl ..\shaders\
if exist triangle.vert.glsl del triangle.vert.glsl
if exist triangle.frag.glsl del triangle.frag.glsl
if exist meshlet.vert.glsl del meshlet.vert.glsl
if exist meshfvf.vert.glsl del meshfvf.vert.glsl
if exist triangle.vert.spv  del triangle.vert.spv
if exist triangle.frag.spv  del triangle.frag.spv
if exist meshlet.vert.spv  del meshlet.vert.spv
if exist meshfvf.vert.spv  del meshfvf.vert.spv

if not exist ..\build mkdir ..\build
pushd ..\build
cl %CompileFlags% ..\code\vulkan_main.cpp kernel32.lib "C:\VulkanSDK\1.2.189.2\Lib\vulkan-1.lib" "C:\VulkanSDK\1.2.189.2\Lib\VkLayer_utils.lib" SDL2main.lib SDL2.lib /link %LinkFlags%
popd
