# PR-03 — Clipping directionnel et OcclusionCache

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/03-occlusion-cache`, créée depuis la tête courante de `V0.1.4` après fusion de PR-01. **Branche cible : `V0.1.4`.** Ne jamais cibler `main`.

**Objectif :** Fournir l'algorithme CPU de clipping/occlusion directionnelle et un cache de résultats facultatif, sans Volume, Model, GPU ni VoxelMesher.

**Prérequis :** [PR-01](PR-01-voxel-foundations.md) avec les huit `Shape::Instance` déjà précalculées. PR-02 (Volume) peut avancer en parallèle.

## Spécifications normatives

- [Modèle voxel](../02-modele-voxel.md)
- [Meshing et occlusion](../03-meshing-occlusion.md)
- [Contrats et validation](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)

## Périmètre exact à réaliser

- [ ] Implémenter **une seule fois** le clipping/occlusion CPU directionnel A → B, réutilisable par PR-04 **avec et sans cache**.
- [ ] Lire les géométries par `Shape::instance(orientation)` ; ne jamais recalculer les huit orientations déjà créées en PR-01.
- [ ] Polygones occultants opaques, frontaux et sur la frontière pertinente ; couverture partielle, union géométrique, interpolation correcte des UV et normales.
- [ ] `OcclusionCache` autonome : clé par identités des deux `Shape::Data`, orientations, direction, opacités géométriques et tolérance. `OcclusionResult` est immuable et conserve les fragments visibles de A et leurs slots locaux.
- [ ] `find/store/clear` ou API équivalente, `ProtectedData<Data>` privé, calcul géométrique hors verrou. Absence distincte d'un résultat valide vide.
- [ ] Fournir le même calcul d'occlusion sans stockage dans le cache, directement réutilisable par le futur Mesher.

## Contrat architectural

`Shape::Data` possède déjà les huit `Shape::Instance`. **Ne pas introduire de `ShapeTransformationCache`**, de liaison de transformation, de copie persistante des géométries orientées ni de deuxième algorithme de clipping. `OcclusionCache` est facultatif pour `VoxelMesher` ; son absence conduira PR-04 à recalculer les résultats d'occlusion, pas les orientations.

`Cell` ne transforme aucune géométrie et ne requiert aucun accès `friend`. Les tests des transformations géométriques appartiennent à PR-01.

## Tests obligatoires

- [ ] A → B distinct de B → A ; entrée absente différente d'un résultat présent contenant zéro fragment.
- [ ] Même résultat pour calcul direct, cache initialement vide et cache déjà renseigné.
- [ ] Occlusion partielle, union des polygones opaques, et absence d'occlusion par faces arrière, translucides ou `innerShell`.
- [ ] UV et normales interpolées, winding CCW, stabilité des fragments et tolérance de clipping.
- [ ] Copies d'`OcclusionResult` valides après `clear()`, réinsertion possible après purge, lecteurs concurrents, identité de `Shape::Data` sans collision ABA.
- [ ] Vérifier que le clipping réutilise les instances de PR-01 sans les modifier.
- [ ] Tests Core Windows/Linux Debug/Release, sans Engine ni OpenGL.

## Critères de livraison

Le clipping CPU est réutilisable sans cache, et `OcclusionCache` ne fait que mémoriser ses résultats. Déclarer les tests réellement exécutés, respecter la revue du propriétaire et ne pas fusionner sans son accord.

**Hors périmètre :** création ou transformation des `Shape::Instance`, Volume, VoxelMesher, Model, GPU, LRU, éviction automatique et ordonnanceur.
