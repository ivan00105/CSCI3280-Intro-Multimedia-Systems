#!/bin/bash

cmd.exe /c "viewSynthesis.exe LF_views -90 120 0" && mv newView.bmp "90 120 0.bmp"
cmd.exe /c "viewSynthesis.exe LF_views -90 90 0" && mv newView.bmp "90 90 0.bmp"
cmd.exe /c "viewSynthesis.exe LF_views -110 100 0" && mv newView.bmp "110 100 0.bmp"
cmd.exe /c "viewSynthesis.exe LF_views -120 100 0" && mv newView.bmp "120 100 0.bmp"
cmd.exe /c "viewSynthesis.exe LF_views -120 120 0" && mv newView.bmp "120 120 0.bmp"
cmd.exe /c "viewSynthesis.exe LF_views -120 90 0" && mv newView.bmp "120 90 0.bmp"