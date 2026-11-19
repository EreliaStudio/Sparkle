# PR27 — Sérialisation voxel dans ByteStream et Message

Aucun nouveau transport, serveur/client, protocole réseau global, monde ou gestionnaire de chunks n'est nécessaire : Sparkle possède déjà ses classes de réseau et ses files.

## Écriture / lecture

Les opérateurs spécialisés sont disponibles en incluant `voxel/volume.hpp`.

```cpp
spk::Message::Writer messageWriter(messageType);
messageWriter << volume;
spk::Message message = std::move(messageWriter).build();

spk::Voxel::Volume::Builder builder(volume.dimensions());
builder.bind(definitions);
auto reader = message.reader();
reader >> builder;
spk::Voxel::Volume reconstructed = builder.build();
```

La même API fonctionne directement avec `spk::ByteStream::Writer` et `spk::ByteStream::Reader` :

```cpp
spk::ByteStream::Writer writer;
writer << volume;
const auto bytes = std::move(writer).build();

spk::Voxel::Volume::Builder builder(volume.dimensions());
builder.bind(definitions);
auto reader = bytes.reader();
builder.load(reader);
const auto reconstructed = builder.build();
```

`spk::Message::Reader` est déjà un alias de `spk::ByteStream::Reader`. Il n'existe donc qu'un seul chemin de décodage pour les deux cas.

## Format du payload Volume (version 1)

Le flux est indépendant de la représentation mémoire C++ et de l'en-tête réseau Message :

| Décalage | Contenu |
| --- | --- |
| 0 | Magic ASCII "SPKV" (4 octets) |
| 4 | Version `uint32` little-endian (1) |
| 8 | Dimensions X, Y, Z, chacune `uint32` little-endian |
| 20 | Cellules `uint32` little-endian dans l'ordre X, puis Y, puis Z |

La taille encodée est `20 + 4 * X * Y * Z` octets. Les cellules utilisent directement
`Voxel::Volume::PackedCell` : 29 bits d'ID de Definition et 3 bits d'orientation.
Aucune palette de rendu, géométrie ou registre de définitions n'est transmise.

Le `Volume::Builder` destinataire doit être construit avec les dimensions attendues
et lié à un `Definition::Registry` compatible. Un ID inconnu, des dimensions
différentes, une version non reconnue ou un flux tronqué produisent `spk::Exception`.

Les lectures sont **atomiques** vis-à-vis du Builder et du curseur Reader :
en cas d'erreur, ils restent inchangés. Une lecture réussie avance le curseur
exactement de la taille de ce Volume, afin de pouvoir sérialiser plusieurs
objets successivement dans un même flux. La méthode `Builder::load(const Volume&)`
et le format de fichiers `loadPacked(path)` restent inchangés.

Le protocole réseau existant gère toujours le framing, les connexions,
le routage et les limites de messages. Les applications peuvent transporter
ces payloads et déterminer les chunks, les révisions ou les destinataires
sans nouvelle couche imposée par le module voxel.
