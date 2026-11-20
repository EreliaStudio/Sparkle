# PR02 — ChunkCollection, ChunkFilter et entités headless
**Couche :** Core/EngineHeadless. **Dépendances :** PR00, PR01. **État :** cahier des charges.

## Possession et modèle
- ChunkCollection<TChunk> possède une std::map<ChunkCoordinate,ChunkEntry> ordonnée lexicographiquement. Chaque entrée contient TChunk **par valeur**, une Entity3D et les contrats d'abonnement nécessaires. Les pointeurs de possession restent internes aux entités si leur API l'exige ; l'utilisateur ne doit pas donner de unique_ptr<TChunk>.
- Une Entity3D racine de collection, et une Entity3D par chunk chargé, enfant de la racine, placée à l'origine en unités du monde. Ces entités existent sans Graphics, pour la physique future.
- Un ChunkFilter<TChunk> : Component3D non propriétaire exposant l'accès rapide au chunk de son Entity3D. Il ne possède pas un deuxième exemplaire du chunk.
- Adresse du TChunk stable pendant la présence de l'entrée, y compris addChunk(coord, nouveauChunk) sur une clé existante : remplacer la valeur **sur place** ou rebinder le ChunkFilter sans aucune fenêtre d'accès périmé. La même Entity3D, ses composants et son identité sont conservés.
- La collection s'abonne à l'événement d'édition de chaque chunk, le relaie immédiatement en donnant sa coordonnée ; sur remplacement, l'ancien abonnement est résilié et le nouvel exemplaire lié. La collection notifie également insertion et remplacement. Un chunk copié ailleurs n'hérite jamais des abonnements de l'original.
- Les setters du chunk déclenchent notifyChunkEdition() eux-mêmes. Aucun besoin de collection.notifyChunkChanged() après un setter normal.
- À removeChunk : rendre le ChunkFilter inaccessible, détruire l'Entity3D et tous ses composants avant de détruire le chunk ; pas de seconde notification générique de suppression. LifeCycle::Destruction permet aux consommateurs de nettoyer leurs ressources.
- Support de la racine hors EngineHeadless, attach/detach/rebind explicites, préservation des entrées et entités. Un système lié à un autre contexte se déconnecte et ne se reconnecte pas implicitement.
- Les mutations sont pilotées par l'appelant, dans le respect des règles de thread de l'Engine ; ChunkCollection n'ajoute pas de vérification de thread. Callback de collection immédiat ; chaque consommateur est libre de regrouper son propre travail.

## API
addChunk(coord,TChunk) accepte un TChunk par valeur ou mouvement ; at(coord) et operator[](coord) renvoient TChunk& ou spk::Exception si absent ; tryGet(coord, TChunk*& chunk) renvoie bool, remplit un pointeur non propriétaire (nullptr si absent) ; surcharge const avec const TChunk*& ; contains(coord). Formes const. begin/end/rbegin/rend const et non const, chaque itération fournit [coord,entry] avec entry.chunk() et entry.entity(), sans mutation des clés ou de la possession.

## Tests
Remplacement sans recréation d'Entity3D ni de composants, stabilité du pointeur et du ChunkFilter, copies indépendantes et abonnements résiliés, absence, ordonnancement, thread attach, destruction ordonnée, Engine headless, abonnés tardifs et reentrance des callbacks.

## Exclusions
Pas de streaming, de meshing ou d'OpenGL. Le remeshing des six voisins appartient à PR08, pas à cette collection générique.
