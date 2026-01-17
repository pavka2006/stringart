@echo off
REM String Art Video Generator Batch Script

setlocal enabledelayedexpansion

set JsonPath=output/result.json
set FPS=30
set OutputFile=output.mp4
set FrameDir=video_frames

echo === String Art Video Generator (Batch) ===
echo.

REM Запускаем программу генерации фреймов
echo Starting frame generation...
video_generator.exe "%JsonPath%" %FPS% "%OutputFile%"

if not exist "%FrameDir%" (
    echo ERROR: Frames directory not found!
    exit /b 1
)

echo.
echo ========== CREATING VIDEO ==========
echo [  0%%] Encoding video with ffmpeg...

REM Вызываем FFmpeg
ffmpeg -framerate %FPS% -i "%FrameDir%\frame_%%06d.png" -c:v libx264 -pix_fmt yuv420p -y "%OutputFile%"

if errorlevel 1 (
    echo ERROR: FFmpeg encoding failed!
    echo Frames are still in: %FrameDir%
    exit /b 1
)

echo [100%%] Video created successfully!
echo ✓ Output file: %OutputFile%
echo.

echo ========== CLEANUP ==========
echo Deleting temporary frames directory...
rmdir /s /q "%FrameDir%"
echo ✓ Frames deleted

echo.
echo ========== SUCCESS ==========
echo ✓ Video ready: %OutputFile%
echo ✓ FPS: %FPS%
echo.
echo All done! Video file is ready to watch.

pause
