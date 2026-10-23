# spk::Publishable

**Header:** [`design_pattern/publishable.hpp`](../../../include/design_pattern/publishable.hpp)  
**Tests:** `tests/TU/Core/srcs/publishable_test.cpp`, `publishable_contract_test.cpp`, `publishable_extended_test.cpp`

Publishable owns the registry of **Parts** used for initialization and incremental publication. It has `Mode::Source` and `Mode::Replicant`.

## Parts

Derive from `Publishable::Part` and implement `_encode(ByteStream::Writer&)` and `_decode(const ByteStream::Reader&)`. A Source Part calls the protected `invalidate()` when its serializable state changes. Registration uses a Part ID, a reference to the Part and `Flags<Publication>`; registered Parts must remain alive for their registration.

## Publication

| Operation | Behavior |
| --- | --- |
| `encodeInitialization()` | Initial state for registered Parts |
| `captureUpdate(Modified / All)` | Capture updates without immediately committing them |
| `commitUpdate(capture)` | Mark the corresponding captured generation as published |
| `encodeUpdate(Modified / All)` | Convenience update encoding |
| `decode(reader)` | Apply encoded sections to a Replicant |

The `Publication::Update` flag controls inclusion in update publications. `Modified` selects changes; `All` sends the available update Parts independently of invalidation. `UpdateCapture` supports publication across recipients while controlling when modified state is committed.

See [multiplayer architecture](../../architecture/multiplayer.md), [create a Publishable](../../guides/create-a-publishable.md), and [EntityHierarchy](EntityHierarchy.md).
