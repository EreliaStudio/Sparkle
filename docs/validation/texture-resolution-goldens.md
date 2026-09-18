# Reviewed texture-resolution golden images

Source: [CI run 9](https://github.com/EreliaStudio/Sparkle/actions/runs/35372809285),
commit `7080cd8ba1f5c84cd5fb4f9f6401bd48294d6076`.
Artifacts: `visual-diffs-Debug` (10559616779), `visual-diffs-Release`
(10559671097), and `coverage` (10559182451).

Before updating references, Debug, Release and coverage each ran 1,161 tests:
1,130 passed and 31 failed only on golden-image comparisons. All eight newly
added resolution/renderer tests passed. Both installed-package/relocation jobs
and formatting passed. All 55 retained actual PNGs were compared across Debug,
Release and coverage: decoded RGBA pixels were identical.

Each reference/actual pair was visually reviewed, including all animation frames,
button states, window/menu layouts, numeric values, radio selections, scrolling
positions, panel corners and switch orientations. New actuals were also compared
with the pre-change run 6 actuals where available. No additional layout, text,
state-selection or missing-content regression was found in this review.

Intentional appearance changes: native-size icons (32/16/8) replace stretched
icons. Enlarged animation-label sprites and the sprite in the combined command
scene now remain centered at 32 pixels. Smaller icons use the generated
premultiplied-alpha area-averaged atlas cells. The whole-atlas ImageLabel likewise
selects a fitting native atlas instead of stretching it. Widget bounds, hit areas,
text and non-icon rendering are not changed by selection.

Pre-existing differences remain for panels, pressed backgrounds, checkbox/radio
edges, scroll areas and toggle-switch textures. The unchanged images below are
pixel-identical to pre-change Mesa CI output and were visually reviewed before
acceptance; this feature did not fix their cross-driver sampling differences.
The existing text/icon overlap in combined PushButton fixtures is present in the
old references as well and is not introduced by this change.

Only the 55 reviewed PNGs below are replaced, directly from the Debug artifact.
No comparison tolerance is widened and no assertions are removed. These references
record the pinned Windows/Mesa CI backend; they do not establish pixel identity
on every vendor GPU. Final CI reruns validate the committed references.

`Previous CI` counts exact changed pixels against run 6 when available. For images
that passed then (and were consequently deleted), the comparison is against the
old reference instead; those rows are explicitly marked `reference`.

| Image | Comparison source | Changed pixels | Accepted PNG SHA-256 |
| --- | --- | ---: | --- |
| `rendering/integration/complete_scene.png` | previous CI | 3509 | `e1e238419bb5234578ec398c802194e90b19812e78d6037b7c8f78c8ba11b95a` |
| `ui/widget/animation_label/alternate_frame.png` | previous CI | 5701 | `a34c10ab02b1848fa0dc8da9705c1038e3b480e6533fc39a7382d65e62d64576` |
| `ui/widget/animation_label/first_frame.png` | previous CI | 10248 | `0931ded922c3d438601de24f72fa0d4104a78af5ee89c89b0bd964dc8d755533` |
| `ui/widget/animation_label/resized.png` | previous CI | 12013 | `0c6403d923883262481067a6884a034fae7cf1fb5ec0de522024cc199ec21feb` |
| `ui/widget/check_box/checked.png` | previous CI | 0 | `0520a355fc0bc37142318eade5342b225a79f743f6a625a476cf385f5e956d20` |
| `ui/widget/checkable_icon_button/alternate_unchecked.png` | reference | 308 | `0168eea706b04e6b882bdb3fd3672e0af16cd9df100571076e209f7abed69c7c` |
| `ui/widget/checkable_icon_button/checked.png` | previous CI | 147 | `4b4086d00cbf115c4b3b5304765706bbf9c8e6d74bf9fc551a70d5a20554a6db` |
| `ui/widget/combo_box/open_popup.png` | previous CI | 0 | `bf8cc1905937fd7905511a0109df65242536702e846d4e4841de613cf1484bce` |
| `ui/widget/icon_button/default_sprite.png` | previous CI | 183 | `3e6477e33805932b94598e46cbbde20031c7a04bbf199e2abed4b18f1f2087c0` |
| `ui/widget/icon_button/large_geometry.png` | previous CI | 147 | `58ca84ae52102d61947b8572986d38bc22e8969db3a55eab6060549d730ef6fb` |
| `ui/widget/image_label/whole_texture.png` | previous CI | 2252 | `809d0619b979915873d43eeb957d30f87f0d4f1df82681557a4b220f64ecb7f5` |
| `ui/widget/image_label/wide_geometry.png` | reference | 5068 | `86fa4e0952d6868b105aa84e8050f52a9fc91bfafe6a6683353e42fd81a81348` |
| `ui/widget/interface_window/custom_padding_menu.png` | previous CI | 121 | `dc64db4542e1fcc5c016d30172e9a23125d5c3f6af7ca93316a37d0ec8faf21e` |
| `ui/widget/interface_window/minimized.png` | previous CI | 207 | `ce4346e704bdc52dd8bb9124235d10161fa795e5c9e5180f604bdecaee239066` |
| `ui/widget/interface_window/normal.png` | previous CI | 207 | `a4e0d75f4f1d897fe917d5f41d2a807341a6ca5df10d3cd111a21678886dfe95` |
| `ui/widget/message_box/standard.png` | previous CI | 207 | `d8eb7f9b367062a1422e56ccb15cae681878c6917e7b6357f67490d00b9a94d4` |
| `ui/widget/numeric_spin_box/negative_integer.png` | previous CI | 235 | `229cad835525b80cfa50b061b04bac5eefe68a1240429f0b400fe5425fbc49ac` |
| `ui/widget/numeric_spin_box/positive_float.png` | previous CI | 235 | `3ccb28fdf3abb6d3a369afd72c5e2b028d452a68d0e6b0e8b700f32fa37133e3` |
| `ui/widget/numeric_spin_box/standard.png` | previous CI | 235 | `0ed0e6fefca1dc7879554fe858534921a776393f3c8dc10415afed2caa751084` |
| `ui/widget/numeric_spin_box/zero_integer.png` | previous CI | 235 | `df6373ba9a700525dd850fcb1a385c210b84c5a2a7b6f3e895e502b683661404` |
| `ui/widget/panel/asymmetric_corners.png` | previous CI | 0 | `1d3918499d34a2caaaa5503235a70aee2c4fb65673960240823e486149d81c70` |
| `ui/widget/panel/small_corners.png` | previous CI | 0 | `54dc3151a6a50de9f66a844317a23104544a900fea9a83a857ba3ebac5e09d0f` |
| `ui/widget/panel/standard.png` | previous CI | 0 | `791398c9ce06cac48e6ac81f2e1373ed6c6034b8c8567c10dbfb423235ff9f3b` |
| `ui/widget/panel/wide_geometry.png` | previous CI | 0 | `2b19d0d932a30f962599fb9423e37cacd2f4e144ceeb2b57decf12db2cb7fa2f` |
| `ui/widget/push_button/icon_only_hovered.png` | reference | 328 | `08a94267293bedd80cfb448d8efbec2032fccfb1e6913c1398638e39e317c528` |
| `ui/widget/push_button/icon_only_pressed.png` | previous CI | 328 | `47066121ffbd5f39dd6c9d05804fa26fd958fabe99df96bb241cbd06395eda46` |
| `ui/widget/push_button/icon_only_released.png` | reference | 328 | `08a94267293bedd80cfb448d8efbec2032fccfb1e6913c1398638e39e317c528` |
| `ui/widget/push_button/standard.png` | reference | 336 | `f62494d36d25a41cbc0e6355deca5f254cf813e5d6393f9d0b8ea6012de32281` |
| `ui/widget/push_button/text_and_icon_hovered.png` | reference | 336 | `f62494d36d25a41cbc0e6355deca5f254cf813e5d6393f9d0b8ea6012de32281` |
| `ui/widget/push_button/text_and_icon_pressed.png` | previous CI | 328 | `654b3f74198fa20cce334a34d639655796c9db965afba037f562e498c0ba7503` |
| `ui/widget/push_button/text_only_pressed.png` | previous CI | 0 | `90223e34f9debf55751599b608ac9c057bbb06b08e17b9c4ebf5aa6199bc1389` |
| `ui/widget/radio_button/standalone_pressed.png` | previous CI | 0 | `ebe01a451cee0ed54997916cf2e9186a184b65653dd37554cefcd4b74a40fef3` |
| `ui/widget/radio_button_group/selected_bottom_left.png` | reference | 21 | `f3f7c03a0d2b8eca8dc166e2c30570e545e63def56b9c095c4f9c8c0a37878db` |
| `ui/widget/radio_button_group/selected_bottom_right.png` | reference | 21 | `1bb9c618acd1ada6a58ba2da909a9478e3bab4d5e64f82b834b0043b4dc439ba` |
| `ui/widget/radio_button_group/selected_top_left.png` | reference | 21 | `26257d30cd22b950f0ee13074e9b043df4335b7f9a5fc649b973a69229703a4a` |
| `ui/widget/radio_button_group/selected_top_right.png` | reference | 21 | `02c60dc72a70b9162c3e2c771419d6edac5851391d8be2672cc6df063de54c3d` |
| `ui/widget/scroll_area/both_scrollbars_scrolled.png` | previous CI | 0 | `b145d896e96182f2d0c01a366019c28f273be44b54077cbb3c02a56816071f73` |
| `ui/widget/scroll_area/horizontal_scrollbar.png` | previous CI | 0 | `c86ece775df00a7deef00ac7a9195c222914adb7325ae74de22a4ef730b3607e` |
| `ui/widget/scroll_area/standard.png` | previous CI | 0 | `4bc0a6fa31c070ffe6d13afe4299437d2cb5da4384a6b7effe1f8e4b68720889` |
| `ui/widget/scroll_area/vertical_scrollbar.png` | previous CI | 0 | `f0c569ace838b92f1fb81ba177678a99a82e84f6eb4c6d3a962ce87247fabf1f` |
| `ui/widget/scroll_bar/horizontal_end.png` | previous CI | 104 | `290a21e156cd011fb253880bb21cfe3a825730fb681326810165c6144d4ac55b` |
| `ui/widget/scroll_bar/horizontal_start.png` | previous CI | 104 | `3dd8f7cfb9dde64604e3b0b910ac661467612b12d1ab625ebf35bf1a4567f237` |
| `ui/widget/scroll_bar/standard.png` | previous CI | 104 | `ddfbc51a02ba25ff6ee74b5372d4cd8df825583e2128f9b4c77108b0e3c39a50` |
| `ui/widget/scroll_bar/vertical_end.png` | previous CI | 104 | `a99c9703111a990234c2e1e1feb947421da1fe6696b898a271f4ff81c475b5cb` |
| `ui/widget/scroll_bar/vertical_start.png` | previous CI | 104 | `7b30ba140a86fa4164a0dd4ec82e5f43f8117226618d1d692bf51a17d214db0d` |
| `ui/widget/spin_box/maximum.png` | previous CI | 235 | `aadcc93206e5f0b9b40e1804f6ca74674606ca713ae929471857707d079dcc20` |
| `ui/widget/spin_box/minimum.png` | previous CI | 235 | `aff07794abef32249342b1269bc0ca4d138d58ad407c5363ec1197fd4ec95eb3` |
| `ui/widget/spin_box/standard.png` | previous CI | 235 | `f7b235976e3c3eeeafda835ae1f7985ab3e04e51e8396774498eb855552bcd3b` |
| `ui/widget/spin_box/zero.png` | previous CI | 235 | `2b289a8b8660cdb7ac037bc584d5a5d76a53df6bb33ba01f28545e790165f283` |
| `ui/widget/toggle_switch/horizontal_half_transition.png` | previous CI | 0 | `f3d0c24b39b6fb1a47828d07ee62565ee71e99a6aa174ead7b9488ebbc8edbab` |
| `ui/widget/toggle_switch/horizontal_off.png` | previous CI | 0 | `bf8ef4d51be15c527f4d40bbc3b2380ab1d7012bbbbf886398569cf2e57d68d6` |
| `ui/widget/toggle_switch/horizontal_on.png` | previous CI | 0 | `bfd52896f302c39ffac70326f767a15513ee69787f2ba1e27128687b591a447a` |
| `ui/widget/toggle_switch/standard.png` | previous CI | 0 | `bfd52896f302c39ffac70326f767a15513ee69787f2ba1e27128687b591a447a` |
| `ui/widget/toggle_switch/vertical_off.png` | previous CI | 0 | `d1eec7e41bbd0082b01a1ec0efa5b2243c1ae33171f487295f502bf60d0b414e` |
| `ui/widget/toggle_switch/vertical_on.png` | previous CI | 0 | `43640a35a5939da0ad64ed6862ca7b8bc6fc6bdc7f7cce54cd4b7e145466daf6` |
