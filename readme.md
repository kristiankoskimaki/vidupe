Vidupe 1.211
------------

Source code: https://github.com/kristiankoskimaki/vidupe  
Windows exe: https://github.com/kristiankoskimaki/vidupe/releases


Vidupe is a program that can find duplicate and similar video files.
Normal duplicate file software only finds identical files. Vidupe looks at the actual video content
regardless of different format and compression used (digital video fingerprinting).

In order for Vidupe to work, FFmpeg (http://ffmpeg.org/) must be installed:
Place ffmpeg.exe in the same folder as Vidupe.exe or in any system directory.



Features:
 - Simple, easy to use graphical user interface for comparing videos.
 - Supports all widely used video formats.
 - Multithreaded operation, makes full use of all available CPU threads.
 - Employs two powerful image comparison methods: pHash (perceptual hashing) and SSIM (structural similarity).
 - Cross-platform development: source code available for compiling on Windows/Linux/macOS.


Vidupe 1.211 (released 2019-09-18) changelog:
 - Fix crash when scrolling mouse wheel outside program window

Vidupe 1.21 (released 2019-09-13) changelog:
 - New thumbnail mode, CutEnds, for finding videos with modified beginning or end. This is default now.
 - Faster comparison
 - Threads won't hang on videos timing out as long as before
 - Tiny amount of videos wrongly marked as broken

Vidupe 1.2 (released 2019-05-27) changelog:
 - Disk cache for screen captures, >10x faster loading once cached
 - Much faster comparison. Comparison window opens faster
 - Threshold modifiers for same/different video durations now work
 - More accurate interpolation
 - Much old C-style code rewritten

Vidupe 1.1 (released 2019-05-05) changelog:
 - Partial disk cache (video metadata only)
 - Faster loading of videos
 - Removed delay between videos after move/delete
 - Videos kept in memory if comparison window accidentally closed
 - Improved zoom


<!---
Usage:  
After starting Vidupe you must enter which folders to scan for video files. Folders can be added by typing them in,
dragging and dropping a folder onto the window or using the folder browser button next to the input box.
All folders must be separated by a semicolon ( ; ).

Comparison is started by pressing the "Find duplicates" button and all video files in selected folders are scanned.
A lengthy search for videos can be aborted by pressing the button again (that now reads Stop).
Note: some videos may be too broken for FFmpeg to read and will be rejected.



Settings:  
The default settings have been chosen to get best results with a minimum amount of false positives.  
Thumbnails:      How many image captures are taken from each video. The larger the number of thumbnails, the slower the scanning of video files is.
                 After deleting all duplicate videos, some additional matching ones may still be found by scanning again with a different thumbnail size.
                 CutEnds compares the beginning and end of videos separately, trying to find matching videos of different length. This is twice as slow.  
pHash:           A fast and accurate algorithm for finding duplicate videos.  
SSIM:            Even better at finding matches (less false positives especially, not necessarily more matches). Noticeably slower than pHash.  
SSIM block size: A smaller value means that the thumbnail is analyzed as smaller, separate images. Note: selecting the value 2 will be quite slow.  
Comparison       When comparing two videos, a comparison value is generated. If the value is below this threshold, videos are considered a match.  
threshold:       A threshold that is too low or too high will either display videos that don't match or none at all.  
Raise threshold: These two options increase/decrease the selected threshold when two videos have almost same length  
Lower threshold: (meaning very likely that they match even if the computer algorithm does not think so).



Disk cache:  
Searching for videos the first time using Vidupe will be slow. All screen captures are taken one by one with FFmpeg and are saved in the file
cache.db in Vidupe's folder. When you search for videos again, those screen captures are already taken and Vidupe loads them much faster.
Different thumbnail modes share some of the screen captures, so searching in 3x4 mode will be faster if you have already done so using 2x2 mode.
A cache.db made with an older version of Vidupe is not guaranteed to to be compatible with newer versions.



Comparison window:  
If matching videos are found, they will be displayed in a separate window side by side, with the thumbnail on top and file properties on bottom.  
Clicking on the thumbnail will launch the video in the default video player installed.  
Scrolling on the thumbnail with the mouse wheel will load a full size screen capture and zoom it, allowing a visual comparison of image quality,  
Clicking on the filename in blue will open the file manager with the video file selected.  
File properties are colour coded:
 - Tan: both videos have same property
 - Green: "better" property
 - Black: "worse" property (or not used)

Prev and next buttons: cycle backwards and forwards through all matching files.  
Delete: Delete the video.  
Move: Move the video to folder of opposite side.  
Swap filenames: Change filenames between videos.



Beware that a poor quality video can be encoded to seem better than a good quality video.  
Trust your eyes, watch both videos in a video player before deleting.
-->


![](https://user-images.githubusercontent.com/46446783/64857475-520d8180-d62d-11e9-9dc6-36889a3e3218.jpg)
![](https://user-images.githubusercontent.com/46446783/64857483-58036280-d62d-11e9-839c-79e2863adfd8.jpg)

Vidupe Copyright (C) 2018-2019 Kristian Koskimäki  
Vidupe is a free software distributed under the GNU GPL.  
Read LICENSE.txt for more information.
