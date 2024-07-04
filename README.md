# 🎲 cgfx2gltf - A basic CGFX exporter

This tool allows you to extract (most) data from CGFX files.
Everything that is extract is placed in a folder named after the original file (without the extension).

This tool wouldn't be possible without the work of many people who have stumbled into this mess before me.
In no way would I have been able to make this tool without the information they provided via Wikis or source code.
I am in no way trying to take credit for their hard labour, I merely want to make a tool that can be easily used in scripts.
A full list of acknowledgments / credits is available below.

## Features

* ⭕ General Metadata
* 🚧 Vertex Data
* 🕓 Material Params
* 🚧 Textures
* 🕓 Camera Infos
* ❌ Look-up Tables
* ❌ Shaders
* 🕓 Lights
* ❌ Fogs
* 🕓 Scenes
* 🕓 Skeletal Animations
* ❌ Texture Animations
* ❌ Visibility Animations
* 🕓 Camera Animations
* ❌ Light Animations
* ❌ Emitter Data

⭕ implemented , 🚧 work in progress , 🕓 on the todo list , ❌ maybe in the future

## Usage

So, for example, the file `some_model.bin` could be extracted like this:

```sh
$ ./cgfx2gltf -v some_model.bin
```

## Acknowledgments

Informational resources:

* [nouwt.com Wiki](https://web.archive.org/web/20150511211029/http://florian.nouwt.com/wiki/index.php/CGFX_(File_Format)): Detailed list of fields and sections in CGFX (web.archive.org)
* [3dbrew.org Wiki](https://www.3dbrew.org/wiki/CGFX): Information on CGFX dictionaries as well as structural information
* [Custom Mario Kart Wiki](https://mk3ds.com/index.php?title=CGFX_(File_Format)): Structural description of CGFX sections and texture formats

Code ported from:

* [gdkchan/Ohana3DS-Rebirth](https://github.com/gdkchan/Ohana3DS-Rebirth)

Libraries used:

* [stb_image](https://github.com/nothings/stb) by Sean Barrett aka "nothings", public domain
* [stb_image_write](https://github.com/nothings/stb) by Sean Barrett aka "nothings", public domain
* [cgltf](https://github.com/jkuhlmann/cgltf) by Johannes Kuhlmann, MIT License

## License

The source code for this application is licensed under the MIT License.
