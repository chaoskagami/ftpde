ftpde
=======

ftpd is originally created by mtheall, and forked as FTP-GMX by another user to add graphical mods and cia builds.

This is upstream with FTP-GMX's changes merged in, plus:

 * Option handling from terminals. Run `ftpde -h` for more info.
 * Extra commands not yet in ftpd and FTP-GMX.
 * Better root handling on linux - now it doesn't serve `/`.
 * Better security.

In terms of methodology, I find a few bits unacceptable. These will be fixed.

Custom Graphics
---------------
Modify the .png files in the `gfx`folder to add your own graphics. These have no effect when building for linux, since it's purely a console tool on linux.

**app_banner:** 
This image will appear on the top screen before you run the application (.3ds and .cia)

**app_bottom:** 
This is the static in-app image on the bottom screen

**app_icon:** 
This is the icon for the .cia, .3ds, and .3dsx

Features
--------
- Appears to work well with a variety of clients, including wget, curl, and gftp.
- Also compiles for Linux, with additional extensions to allow use in a more serious and secure manner as an actual ftp server for desktop.
- Supports multiple simultaneous clients. The 3DS itself only appears to support enough sockets to perform 4-5 simultaneous data transfers, so it will help if you limit your FTP client to this many parallel requests.
- *Your own* cutting-edge graphics on 3DS. Defaults are non-flashy, but hey, you can get fancy.

Before building
---------------

1) Install and set up devkitARM. I recommend being a crazy bastard and building devkitARM yourself, but then again, I run LFS.

2) Install the latest [ctrulib](https://github.com/smealum/ctrulib/tree/master/libctru). No, devkitPro's will not suffice. You need the development version. Any reports filed with non-dev versions will be completely ignored.

3) Install [sf2dlib](https://github.com/xerpi/sf2dlib)

4) Install portlibs, preferrably from the 'portlibs' subdir of this repo. dkpro's contain known security issues - you want your 3ds to run YOUR code, not be part of a bitcoin mining botnet, right?

5) Install [sfillib](https://github.com/xerpi/sfillib)

6) Precompiled portlibs? No. Go home, you're drunk.

How to build
------------
1) Learn to build other things. No help will be provided if you can't operate a devkit.

2) Run 'make' or 'gmake'.

This will produce the following files:

 * ftpde-VER.3dsx - 3dsx relocatable for hbmenu-style loaders.
 * ftpde-VER.smdh - Fancy graphics for hbmenu.
 * ftpde-VER.cia  - Installable CTR file. Zero-keyed/No crypto.

Implemented commands
--------------------

- ABOR
- APPE
- CDUP
- CWD
- DELE
- FEAT
- HELP
- LIST
- MDTM
- MKD
- NLST
- NOOP
- OPTS
- PASV
- PORT
- PWD
- QUIT
- REST
- RETR
- RMD
- RNFR
- RNTO
- STAT
- STOR
- SYST
- XCUP
- XCWD
- XMKD
- XPWD
- XRMD

New commands in ftpde
---------------------

- SIZE (RFC 3659)

Stubbed Commands that need proper functionality
-----------------------------------------------

- ALLO - fallocate please
- MODE - Only stream is supported. Compressed and block would be nice. 
- PASS - Unacceptable to be stubbed, need access control.
- STRU - 
- TYPE - ASCII mode is needed. Thankfully binary is the one implemented.
- USER - See PASS.

NYI, but should be implemented
------------------------------

- STOU
- HOST
- LANG
- LPRT
- MDTM - Last modified time in format "YYYYMMDDhhmmss"
- MLSD
- NOOP
