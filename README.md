ftpde
=======

ftpd was originally created by mtheall. A fork - FTP-GMX - was created which added CIA builds and customizable graphics, though it stagnated and also broke native builds.

This is upstream ftpde with FTP-GMX's (working) changes merged in, plus:

 * Option handling from terminals. Run `ftpde -h` for more info.
 * Extra commands not yet in ftpd and FTP-GMX.
 * Better root prefix handling.
 * Better security, including user/pass and proper write rejection.
 * Configurable behavior.
 * Color on, color off!

Configuration files
-------------------

For an example config file, see `extra/ftpde.conf`.

Custom Graphics on 3DS
----------------------

You have two choices, depending on your control freak level. One requires building from source code.

Graphics (Simple)
-----------------

The only file which can be replaced at runtime is 'app_bottom.png'. See extra/ftpde.conf for info.

Graphics (Hacker method)
-----------------------------------------------

Modify the .png files in the `gfx` folder to add your own graphics. These have no effect when building for a POSIX system, since it's purely a console tool.

** app_banner ** 
This image will appear on the top screen when the application is highlighted (.cia)

** app_bottom ** 
This is the builtin static in-app image on the bottom screen.

** app_icon ** 
This is the icon for the .cia and .3dsx.

Features
--------
 * Appears to work well with a variety of clients, including wget, curl, and gftp.
 * Also compiles for most POSIX systems, with additional extensions to allow use in a more serious and secure manner as an actual ftp server for desktop. This has been mostly tested on linux, but even mingw might work.
 * Supports a configuration backend which allows changing some parameter defaults.
 * Supports user credentials.
 * Supports multiple simultaneous clients. The 3DS itself only appears to support enough sockets to perform 4-5 simultaneous data transfers, so it will help if you limit your FTP client to this many parallel requests.
 * Custom graphics on 3DS. Defaults are non-flashy, but hey, you can get fancy.

Reporting issues
----------------

 * Please check if an issue is reported here. Also check mtheall/ftpd and FTP-GMX first. Don't dupe.

 * If you file an issue which is a feature request I will either consider it, or close it immediately and tell you why.

Before building
---------------

** For 3DS **

Install and set up devkitARM.

You also need the following host tools:

 * [makerom](https://github.com/profi200/Project_CTR)

 * [bannertool](https://github.com/Steveice10/bannertool)

There's a bug on linux with makerom - check the issue tracker.

Additionally, I don't recommend building with the bundled libyaml and polarssl if you're a security freak. libyaml is easy to unbundle, but polarssl needs some changes. I recommend being a crazy bastard and building everything yourself.

You'll need the following target libs:

 * Git version of [ctrulib](https://github.com/smealum/ctrulib/tree/master/libctru).

 * [sf2dlib](https://github.com/xerpi/sf2dlib)

 * portlibs, preferrably from the `portlibs` subdir of this repo. dkpro's contain known security issues. You need at minimum zlib, libpng, and libconfig.

 * [sfillib](https://github.com/xerpi/sfillib)

** For POSIX systems **

Make sure you have the dev package for your system, as well as the headers. I'll not provide specific instructions, since I assume enough technical competency to figure this out. You need at minimum a functional gcc, libpng, zlib, and libconfig.

How to build
------------

Either `make 3ds` or `make posix` depending on how you plan to use this. With `make posix`, you may want to set `SYSCONFDIR` if configuration should not be in `/etc`.

`make 3ds` will produce the following files:

 * ftpde-VER.3dsx - 3dsx relocatable for hbmenu-style loaders.
 * ftpde-VER.smdh - Fancy graphics for hbmenu.
 * ftpde-VER.cia  - Installable CTR file. Zero-keyed/No crypto, so you'll need CFW.
 * ftpde-VER.3ds  - Encrypted 3DS rom file. No support, I don't own a gateway.

`make posix` will produce the following files:

 * ftpde          - Binary for current platform.

How to install
--------------

** 3DS **

You need to copy the files to your 3DS. Alternatively, you can directly install if you're running CFW and have FBI available with: `make HOST=<3ds IP> fbi-install`

That said, you need a config file at `sd:/ftpde.conf`. Not up for debate, because anonymous has no write permissions. You'll need to set up a proper user, as well, since the default combination is not secure.

** POSIX **

`make install`

You can optionally set `PREFIX` to specify the install prefix, and `SYSCONFDIR` to set the configuration path. Default is `PREFIX=/usr/local`. and `SYSCONFDIR=/etc`

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
- SITE
  - HELP

New or enhanced commands in ftpde
---------------------------------

- SIZE - RFC 3659
- PASS - Proper credential support
- USER - Proper credential support
- PORT - Now only accepts same-IP connections to prevent a subclass of connection stealing.

In progress
-----------

Commands that have problems
---------------------------

- LIST - Needs proper user support, or at very least to specify proper relative permissions. Can actually do `ls params FILE` on posix. This is only relevant on posix.

Stubbed Commands that need proper functionality
-----------------------------------------------

- ALLO - Should run fallocate if available.
- MODE - Only stream is supported. I'd like to implement the 'Z' extension mode to save bandwidth.
- STRU - 
- TYPE - ASCII mode is needed. Thankfully binary is the one implemented.
- STOU
- SITE CHMOD - Should function as chmod(1) on a system which supports it. Currently 200 stub.

NYI, but should be implemented
------------------------------

- HOST
- LANG
- LPRT
- MLSD

Planned Features
----------------

- Load config file to memory rather than re-read from disk, SIGUSR1 to reload or configuration option to watch it.
- IP whitelist support (high priority)
- Rate limiting (possibly)
- Load app_bottom at runtime rather than bin2o'd.
- Random credentials - only one session is allowed to be open, and credentials are generated on start, and regenerated after close. So that a 3ds can be used *mostly* securely in public places.
- Disable overwriting ftpde.conf and app_bottom.png via config option. This is insecure. (high prio)
