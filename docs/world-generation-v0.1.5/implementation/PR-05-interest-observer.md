# PR05 — WorldObserver et domaines d'intérêt
**Couche :** Core/EngineHeadless. **Dépendances :** PR01. **État :** cahier des charges.

## Contrat
- WorldObserver dérive de Component3D : seul un objet portant volontairement ce composant demande des chunks. L'EngineHeadless et Registry existants assurent sa découverte ; aucune nouvelle registry globale.
- ChunkInterest contient un rayon entier non négatif distinct sur X/Y/Z. L'ensemble demandé est la boîte inclusive centrée sur la coordonnée en chunk de la position monde de l'observateur.
- Un observateur se déplaçant à l'intérieur du même chunk **ne déclenche aucune reconstruction des intérêts** ; lorsqu'il franchit une frontière de chunk, la nouvelle coordonnée centre devient pertinente.
- Modification du rayon, ajout/suppression/détachement/rattachement d'un observateur déclenchent aussi la remise à jour des intérêts.
- Plusieurs observateurs autorisés. Pour la v0.1.5, un seul monde/collection par Engine ; aucun filtrage WorldUUID.
- Le WorldObserver n'effectue ni génération, ni rendu, ni calcul d'ordonnancement : il fournit position et rayon.

## Tests
Régistration automatique Component3D/EngineHeadless, positions négatives, rayon (4,2,4) = 405 coordonnées, changement de position intra-chunk sans changement, franchissement de frontière, changement de rayon, deux joueurs et entités non observatrices.

## Exclusions
Pas de frustum, de propagation flood fill, de monde multiple, ni de connexion réseau.
