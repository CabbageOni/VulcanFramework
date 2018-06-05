@echo off

del "desktop.ini" /s /q
del "*.db" /s /q

del "Debug\*" /s /q
rmdir "Debug" /s /q

del "Release\*" /s /q
rmdir "Release" /s /q

del "x64\*" /s /q
rmdir "x64" /s /q

del "VulkanFramework\Debug\*" /s /q
rmdir "VulkanFramework\Debug" /s /q

del "VulkanFramework\Release\*" /s /q
rmdir "VulkanFramework\Release" /s /q

del "VulkanFramework\x64\*" /s /q
rmdir "VulkanFramework\x64" /s /q

#pause