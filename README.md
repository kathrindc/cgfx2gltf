# 🎲 cgfx2gltf - A basic CGFX exporter

This tool allows you to extract (most) data from CGFX files.

## Features

|||
|-|-|
|General Metadata|⭕|
|Vertex Data|🚧|
|Material Params|🕓|
|Textures|🚧|
|Camera Infos|🕓|
|Look-up Tables|❌|
|Shaders|❌|
|Lights|🕓|
|Fogs|❌|
|Scenes|🕓|
|Skeletal Animations|🕓|
|Texture Animations|❌|
|Visibility Animations|❌|
|Camera Animations|🕓|
|Light Animations|❌|
|Emitter Data|❌|

⭕ implemented \
🚧 work in progress \
🕓 on the todo list \
❌ maybe in the future

## Usage

So, for example, the file `some_model.bin` could be extracted like this:

```sh
$ ./cgfx2gltf -v some_model.bin
```

## License

* [cgltf](https://github.com/jkuhlmann/cgltf) by Johannes Kuhlmann, MIT License
