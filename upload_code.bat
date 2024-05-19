picotool reboot -f -u
timeout 2
picotool load -u "build/PicoLibraries.uf2"
picotool reboot
timeout 5
