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
- Also compiles for most POSIX systems, with additional extensions to allow use in a more serious and secure manner as an actual ftp server for desktop. This has been mostly tested on linux, but even mingw might work.
- Supports multiple simultaneous clients. The 3DS itself only appears to support enough sockets to perform 4-5 simultaneous data transfers, so it will help if you limit your FTP client to this many parallel requests.
- *Your own* cutting-edge graphics on 3DS. Defaults are non-flashy, but hey, you can get fancy.

Before building
---------------

1) Install and set up devkitARM. You also need [makerom](https://github.com/profi200/Project_CTR) and [bannertool](https://github.com/Steveice10/bannertool). There's a bug on linux with makerom - check the issue tracker. Additionally, I don't recommend building with the bundled libyaml and polarssl if you're a security freak. libyaml is easy to unbundle, but polarssl needs some changes. I recommend being a crazy bastard and building everything yourself.

2) Install the latest [ctrulib](https://github.com/smealum/ctrulib/tree/master/libctru). No, devkitPro's will not suffice. You need the development version. Any reports filed with non-dev versions will be completely ignored.

3) Install [sf2dlib](https://github.com/xerpi/sf2dlib)

4) Install portlibs, preferrably from the 'portlibs' subdir of this repo. dkpro's contain known security issues - you want your 3ds to run YOUR code, not be part of a bitcoin mining or DDoS botnet, right? Could happen. You can never be too paranoid.

5) Install [sfillib](https://github.com/xerpi/sfillib)

6) Precompiled portlibs? No. Go home, you're drunk.

How to build
------------
1) Learn to build other things. No help will be provided if you can't operate a devkit.

2) Either `make 3ds` or `make posix` depending on how you plan to use this.

This will produce the following files:

 * ftpde-VER.3dsx - 3dsx relocatable for hbmenu-style loaders.
 * ftpde-VER.smdh - Fancy graphics for hbmenu.
 * ftpde-VER.cia  - Installable CTR file. Zero-keyed/No crypto, so you'll need CFW.
 * ftpde          - Binary for current platform.

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

In progress
-----------

- PASS - Partially implemented, needs configuration
- USER - Partially implemented, needs configuration

Commands that have problems
---------------------------

- LIST - Doesn't parse unix command syntax, like -aL. If the ls command is available, we shouldn't be enumerating ourselves.

Stubbed Commands that need proper functionality
-----------------------------------------------

- ALLO - fallocate please
- MODE - Only stream is supported. Compressed and block would be nice. 
- STRU - 
- TYPE - ASCII mode is needed. Thankfully binary is the one implemented.

NYI, but should be implemented
------------------------------

- STOU
- HOST
- LANG
- LPRT
- MLSD
