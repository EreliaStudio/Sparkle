# Sparkle v0.1.4 — PR27: réseau voxel headless

## Structures et responsabilité

spk::Voxel::World indexe des chunks identifiés par coordonnées signées (ChunkPosition).
Chaque chunk contient un Voxel::Volume immuable et une révision monotone ; il
n'existe pas de seconde représentation de ses cellules pour le transport.
Les chunks sont uniformes et limités à 65 536 cellules.

Les ressources Shape, Definition et leurs registres sont chargées localement
avant la connexion. Client et serveur doivent employer les mêmes associations
entre Definition IDs et définitions complètes. Ils fournissent un catalogueId
non nul lors de la construction. L'application doit dériver une valeur stable
de son manifest d'assets versionnés. Cette valeur est un contrat de compatibilité,
pas une signature cryptographique ni une vérification automatique de fichiers.
Déclarer une valeur identique avec des contenus divergents n'est pas pris en charge.
Les indices de palettes graphiques ne sont jamais envoyés.

Le mesher, les géométries, les palettes, OpenGL et les invalidations GPU
restent locaux à la partie graphique.

## Transport et thread

Le protocole utilise les transports TCP asynchrones existants spk::Server et
spk::Client, sans nouvelle dépendance. Les workers réseau ne modifient pas
le monde. L'application appelle Server::update() et Client::update() sur le
thread propriétaire des états voxel. takeChangedChunks() fournit une liste
unique des chunks modifiés ; le renderer doit reconstruire le chunk concerné
et, si nécessaire, ses voisins pour l'occlusion aux frontières.
Toutes les opérations World/Network sont mono-thread logique.

## Protocole binaire version 1

Les entiers sont explicitement encodés en little-endian, jamais copiés directement
depuis les structs C++. Tous les messages débutent par une version u8 égale à 1.

| Type de message | Champs après la version |
| --- | --- |
| Hello (0x56580001) | u64 catalogueId, u32 dimensionX/Y/Z |
| Snapshot (0x56580002) | i32 chunkX/Y/Z, u64 revision, u32 count, count fois u32 packedCell |
| Patch (0x56580003) | i32 chunkX/Y/Z, u64 previous, u64 next, u32 count, count fois (u32 localX/Y/Z, u32 packedCell) |
| EditRequest (0x56580004) | i32 chunkX/Y/Z, u64 expectedRevision, même lot de cellules |
| Ready (0x56580005) | aucun champ |
| ResyncRequest (0x56580006) | i32 chunkX/Y/Z |
| Error (0x56580007) | u8 code Error |

Le packedCell reprend exactement le codec Volume::PackedCell : 29 bits
pour le Definition ID et 3 bits pour l'orientation, sans remappage local.
Chaque ID est validé auprès de Definition::Registry avant application.
Les limites sont de 1 MiB par message, 4096 cellules par édition et 65536
cellules par chunk. Une révision part de 1 et augmente de 1 à chaque
modification effective ; un patch exige next == previous + 1.

Les champs manquants, excédentaires, IDs inconnus, positions hors limites
et doublons sont rejetés. Toute lacune dans les révisions provoque un
ResyncRequest et le serveur renvoie un Snapshot. Une reconnexion reconstruit
le monde local à partir de zéro.

## API publique

- Server::setChunk(position, volume) publie ou remplace un chunk et diffuse son snapshot.
- Server::edit(position, edits) applique un lot atomiquement et diffuse un patch.
- Client::requestEdit(position, edits) propose une modification sans modifier immédiatement l'état local.
- Server::setEditValidator(callback) permet de refuser les demandes client selon les règles métier.
- Server::update() et Client::update() consomment les messages sur le thread appelant.
- World::chunk(position), World::revision(position) et World::chunks() permettent de consulter l'état.
- takeChangedChunks() permet au rendu de consommer les invalidations.

Par défaut, tous les clients synchronisés peuvent modifier les chunks
existants. Les jeux nécessitant des permissions doivent installer leur
propre callback de validation métier.

## Périmètre exclu

- Streaming spatial par zones d'intérêt ou chargement différé : l'état initial
  contient tous les chunks publiés.
- Suppression dynamique de chunks, monde infini, persistance disque.
- Transfert à distance des registres Shape/Definition et des matériaux graphiques.
- Authentification, chiffrement et mécanismes de protection pour Internet public.
- Model3D, animations, caméra, prédiction, réconciliation et matchmaking.

Les tests d'intégration fonctionnent sur loopback et ne nécessitent aucun serveur externe.
