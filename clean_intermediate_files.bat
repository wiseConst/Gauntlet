@echo off

set dirsToRemove=*.vs Intermediate Binaries bin bin-int # i added bin and bin-int because some of 3rdparties are added from cherno
set filesToRemove=*.sln *.vcxproj *.vcxproj.user *.vcxproj.filters

for %%a in (%dirsToRemove%) do (
	rmdir /S /Q %%a
)

for %%a in (%filesToRemove%) do (
	del %%a
)

cd Sandbox
for %%a in (%dirsToRemove%) do (
	rmdir /S /Q %%a
)
cd ../

cd Eclipse
for %%a in (%filesToRemove%) do (
	del %%a
)
cd ../


cd Eclipse/vendor

cd vma
for %%a in (%dirsToRemove%) do (
	rmdir /S /Q %%a
)

for %%a in (%filesToRemove%) do (
	del %%a
)
cd ../

cd GLFW
for %%a in (%dirsToRemove%) do (
	rmdir /S /Q %%a
)

for %%a in (%filesToRemove%) do (
	del %%a
)
cd ../

cd imgui
for %%a in (%dirsToRemove%) do (
	rmdir /S /Q %%a
)

for %%a in (%filesToRemove%) do (
	del %%a
)
cd ../

#PAUSE
#echo %cd%