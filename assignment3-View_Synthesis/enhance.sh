#!/bin/bash

cmd.exe /c "viewSynthesis.exe LF_views 0 0 100 100" && mv newView.bmp "0 0 100 100.bmp"
cmd.exe /c "viewSynthesis.exe LF_views 0 0 100 50" && mv newView.bmp "0 0 100 50.bmp"
cmd.exe /c "viewSynthesis.exe LF_views 0 0 100 500" && mv newView.bmp "0 0 100 500.bmp"
cmd.exe /c "viewSynthesis.exe LF_views 0 0 1000 100" && mv newView.bmp "0 0 1000 100.bmp"
cmd.exe /c "viewSynthesis.exe LF_views 100 -100 500 100" && mv newView.bmp "100 -100 500 100.bmp"
