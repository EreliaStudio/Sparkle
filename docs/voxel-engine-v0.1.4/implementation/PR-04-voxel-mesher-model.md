# PR-04 — VoxelMesher, clipping et Model

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/04-voxel-mesher-model`, créée depuis la tête courante de `V0.1.4` après fusion de ses prérequis. **Branche cible : `V0.1.4`.** Ne jamais cibler `main`.

**Objectif :** Produire un Model voxel immuable à partir d'un Volume, en utilisant les `Shape::Instance` de PR-01 et le clipping de PR-03.

**Prérequis :** [PR-02 — Volume](PR-02-voxel-volume.md) et [PR-03 — Clipping et OcclusionCache](PR-03-occlusion-cache.md).

## Spécifications normatives

- [Modèle voxel](../02-modele-voxel.md)
- [Meshing, visibilité et occlusion](../03-meshing-occlusion.md)
- [Ombres](../08-lumieres-et-ombres.md)
- [Contrats et validation](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)

## Périmètre exact à réaliser

- [ ] `VoxelMesher` : bake synchrone complet d'un Volume, **sans obligation de fournir un `OcclusionCache`**.
- [ ] Sans cache lié : recalculer les occlusions par le clipping CPU commun de PR-03 ; avec cache : consulter/mémoriser les résultats. Les résultats des deux chemins doivent être identiques.
- [ ] Obtenir la géométrie orientée uniquement via `Shape::instance(orientation)`, sans rotation ou flip à la volée et sans cache de transformation supplémentaire.
- [ ] Voisinage extérieur `Empty`, `Occupied`, `Unknown` ; visibilité directionnelle et règle globale des `innerShell`.
- [ ] Tolérance de clipping configurable `1e-5` par défaut et incluse dans les clés de cache.
- [ ] `Model` immuable : meshes visuels opaques/translucides, mesh de profondeur `ShadowMap::Mesh` et slots sémantiques triés lexicographiquement.
- [ ] Utiliser le `spk::Mesh<TVertex>` existant ; préserver/interpoler UV et normales, indices et winding.
- [ ] Employer l'opacité des slots géométriques, non l'alpha des textures, pour culling, classification et ombres.

## Contrat du cache facultatif

Une instance de `VoxelMesher` sans `OcclusionCache` fonctionne normalement. Le cache externe peut être emprunté et partagé entre plusieurs Meshers, mais il n'est jamais créé implicitement. La suppression de la liaison rétablit le mode sans cache ; sa durée de vie pendant le bake relève de l'appelant. **Ne pas dupliquer l'algorithme de clipping**, déjà livré par PR-03.

## Tests obligatoires

- [ ] `bake()` **sans cache** réussit et produit le même Model que les chemins avec cache vide ou prérempli ; répéter un bake sans cache ne réutilise pas un résultat mémorisé.
- [ ] Cache lié puis retiré : le Mesher reste fonctionnel ; deux Meshers peuvent partager un cache.
- [ ] Réutilisation des `Shape::Instance` précalculées pour toutes les orientations, sans transformation ou mutation de leur géométrie.
- [ ] Clipping partiel, union des occultants, respect des faces avant, opacités, UV et normales.
- [ ] Six voisins, trois états extérieurs, règle globale des `innerShell`.
- [ ] Opacité 1 / inférieure à 1, absence d'inférence depuis les textures, mesh d'ombre purement géométrique.
- [ ] Slots masqués conservés, collision nom/opacité rejetée et ordre lexicographique déterministe.
- [ ] Anciens Models immuables ; tests pertinents Debug/Release sur les plateformes prises en charge.

## Critères de livraison

Premier Model voxel vérifiable numériquement avec ou sans cache d'occlusion. Déclarer les tests réellement exécutés, corriger les violations de contrat et obtenir la validation du propriétaire avant fusion.

**Hors périmètre :** rendu GPU, greedy meshing, rebake incrémental, cache de transformation externe et toute PR ultérieure.
