# WPC - Desktop and lock screen wallpaper setter

## Introduction

WPC (wallpaper changer) is a program that allows the user to set wallpapers for both the desktop and lock screen. It is a simple and easy-to-use application that lets you personalize your computer and make it more visually appealing.

The program supports various image formats and makes it easy to change the wallpaper with a user-friendly interface.

## Features

![Demo](wpc_demo.gif)

- Set wallpapers for both the *desktop* and *lock screen*.
- Store background source directory along with set backgrounds for monitors in a *config file*.
- Ability to *change the source directory* directly from the GUI.
- No need for `sudo` (previously required for manually editing `/etc/lightdm/*`).
- Clean separation between *desktop wallpaper* handling and *lock screen wallpaper* handling.
- Support for common image formats (JPEG, PNG, BMP, etc.).
- [Multiple wallpaper modes](BG_MODES.org)

## Planned features/todo list

- Add Wayland support
- Migrate from JSON to toml for config
- Improve security for desktop manager helper
- Implement support for other desktop manager than LightDM + lightdm-gtk-greeter
- Implement caching of thumbnails for wallpapers in the UI
- Add -d flag which should behave like -b flag (which starts with no GUI and sets backgrounds and exits) but keep the software running and switch backgrounds according to algo on a configurable interval

## Installation

To install WPC, follow these steps:

```bash
git clone https://github.com/webdevred/wpc
cd wpc
gcc nob.c -o nob
./nob
```

Alternatively, you can use the legacy Makefile:
```bash
git clone https://github.com/webdevred/wpc
cd wpc
make
```

Both build systems should work, but nob is the preferred method.

## Usage

You can create this config file in ~/.config/wpc/settings.json before you start the program.
If you do not create a config file then source directory will be set to your pictures directory in your home directory.

```js
{
    "sourceDirectoryPath" : "/mnt/HDD/backgrounds/",
    "monitorsWithBackgrounds": []
}
```

1. Set Desktop Wallpaper:
   - Open the program by running wpc in your terminal
   - Browse to select an image file from your computer
   - The image will now applied for the correct monitor

2. Set Lock Screen Wallpaper:
   - Select the "Lock Screen" tab.
   - Follow the same process as the desktop wallpaper to select an image file.

## System requirements

Requirements:
- libxrandr-dev
- libx11-dev
- libgtk-4-dev
- libmagic-dev
- libcjson-dev
- libmagickwand-dev
- imagemagick

Install the requirements like this:

```bash
sudo apt update && sudo apt install -y libxrandr-dev libx11-dev libgtk4-dev libcjson-dev libmagickwand-dev
```

## License

WPC is licensed under the MIT License. See the LICENSE file for more information.
