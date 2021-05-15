# OniFoxed

## What's this?

This is a modified variant of the recently
leaked [Oni source code](https://archive.org/details/oni-source) so that it compiles
under Microsoft Visual Studio 2019 with some
minor fixes/changes.

The code is aligned to be run against the [Windows
demo version](https://archive.org/details/OniDemo) of the game, and because of this there are quite a few
issues when running this against the final retail
version of the game.

## What are you trying to accomplish?

Well, I wanted to get it compiling and running; I've
accomplished this.

Looking into the future I'd love to see the following,
but time is an issue for me - contributions are welcome!

- [ ] Migrate the projects to CMake
- [ ] Fix alignment issues against the final PC release
- [ ] Linux and macOS support
- [ ] Modernized renderer, with support for shaders
- [ ] 64-bit compatibility
