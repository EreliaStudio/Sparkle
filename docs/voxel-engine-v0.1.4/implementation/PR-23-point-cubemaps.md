# PR-23 — Cubemaps PointLight et PCF inter-faces

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/23-point-cubemaps`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Étendre les cartes statique/finale aux PointLight omnidirectionnelles.

**Prérequis :** [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-17](PR-17-light-components.md), [PR-21](PR-21-shadowresults-surfaces.md), [PR-22](PR-22-shadowmaps-spot-directional.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] TextureCubeMap D32F avec six faces 90°
- [ ] attachement explicite de chaque face dans FBO depth-only
- [ ] copie depth statique→finale face par face
- [ ] publication uniquement après six faces
- [ ] PCF 3x3 avec reprojection inter-faces.

## Instructions techniques et intégration

Réutiliser Texture/Framebuffer/GPUResourceCollection ; égalités d'axe X>Y>Z; neuf taps manuels nearest, référence et biais par face; point P==L visible sans sampler, near historique sans ombre; ni geometry shader layered ni 6 noyaux.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Six faces sans fuite
- [ ] arêtes et coins cubemap
- [ ] ref near et centre exact
- [ ] 1024 D32F par défaut et resize per-light
- [ ] fail d'une face => aucun résultat partiel
- [ ] deux surfaces gardent instances distinctes.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Ombres Point visibles depuis toutes directions, sans publication des cartes incomplètes.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

raffinement progressif, filtering par valeur profondeur inter-face.

## Consigne de passation

Confier **uniquement PR-23** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
