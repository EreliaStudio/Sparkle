# Texture resolution families

`Texture::addResolution` accepts a texture, an encoded image span, or a file path.
Levels must be nonempty CPU color textures with the same format and aspect ratio;
duplicate sizes are rejected. Register images with the same normalized UV layout.

```cpp
auto icons = spk::SpriteSheet::open("icons32.png", {10, 10});
icons.addResolution("icons16.png");
icons.addResolution("icons8.png");
auto selected = icons.resolution(spk::Vector2UInt{18, 18});
```

`Texture` and `Image` compare whole-image dimensions; `SpriteSheet` compares
per-sprite dimensions and requires integral cells. The predicate overload takes
`const Texture &` candidates and selects the largest accepted level. Selection
returns a lightweight `Texture` value sharing resource state and the resolution
family. The selected texture can select larger levels, even after the original
object is destroyed. Its size overload compares whole-image dimensions; retain
the original sprite sheet for sprite-aware requests.

The nested `ResolutionSet` owns GPU resource handles, whose states do not refer
back to the set. There is no shared-ownership cycle. Family additions are visible
to existing family views. Add levels on the owning/update thread, not concurrently
with selection. Pixel edits detach that view from its family; existing views and
snapshots retain their previous resources. Cloning creates an independent family
collection and clones the currently selected resource state.

ImageRenderCommand's texture-pointer overload chooses a level for the UV section
and centers its native dimensions on integer pixel coordinates. The handle overload
renders the already-selected resource at the supplied size. Textures without a
family preserve existing scaling. When no level fits, selection falls back to the
current level and image drawing preserves the requested destination size.

Default widgets use 32, 16 and 8 pixel icons. Their 320, 160 and 80 pixel atlases
are embedded by the existing resource builder. Regenerate the small PNGs with
`python tools/assets/generate_icon_resolutions.py` (Pillow required only for asset
generation). The generator averages premultiplied-alpha pixels within each sprite;
it does not filter across atlas-cell boundaries. No runtime resizing or filesystem
access is needed for default icons.
