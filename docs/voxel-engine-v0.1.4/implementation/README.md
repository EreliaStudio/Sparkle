# Sparkle v0.1.4 — Roadmap après PR18 (26 fiches historiques)

**PR01–PR13 et PR15–PR18 fusionnées dans `V0.1.4` ; PR14 retirée (#64 fermée sans fusion) ; PR19–PR26 à réaliser.** Les API réellement fusionnées priment sur les préparations historiques. Voir [état PR01–08](IMPLEMENTED-01-08.md), [PR13 livrée](PR-13-voxel-render-first.md) et [décision de retrait PR14](PR-14-assembly-render.md).

**Références :** [spécification normative](../../voxel-engine-v0.1.4-specification.md) · [PR documentaire #37](https://github.com/EreliaStudio/Sparkle/pull/37) · [dépendances](DEPENDENCIES.md) · [couverture des fonctionnalités](COVERAGE.md) · [prompt pour une IA](AI-HANDOFF.md).

## Fiches

| Fiche | Dépendances minimales (numéros de fiches) | Résultat attendu |
|---|---|---|
| [PR-01 — Shape, Definition et Cell](PR-01-voxel-foundations.md) | — | Constituer le modèle voxel public sans EngineHeadless et sans OpenGL. |
| [PR-02 — Volume dense et Builder](PR-02-voxel-volume.md) | 01 | Éditer une grille voxel puis publier des snapshots immuables. |
| [PR-03 — Clipping directionnel et Occlusion::Cache](PR-03-occlusion-cache.md) | 01 | Fournir le clipping CPU commun et un cache de résultats facultatif. |
| [PR-04 — Mesher et Geometry](PR-04-voxel-mesher-model.md) | 02, 03 | Produire `Voxel::Geometry` depuis `Volume` avec ou sans `Occlusion::Cache`. |
| [PR-05 — Asset::Manager et Asset::Factory typés](PR-05-asset-manager.md) | — | Charger et posséder des assets C++ génériques à identité stable. |
| [PR-06 — Material multi-rôles, Texture et SpriteSheet](PR-06-materials-json.md) | 05 | Construire des Materials immuables via Builder ou JSON. |
| [PR-07 — Palette et compatibilité Geometry](PR-07-voxel-palette.md) | 04, 06 | Créer des palettes immuables et vérifier les slots de `Voxel::Geometry`. |
| [PR-08 — Model3D Blueprint et Object](PR-08-assembly-hierarchy.md) | — | Définir des hiérarchies immuables, leurs occurrences articulées et le cache de vues graphiques génériques. |
| [PR-09 — Engine : EngineHeadless et Contributors](PR-09-rendering-engine-inheritance.md) | — | Moderniser l'API moteur graphique tout en préservant le moteur headless. |
| [PR-10 — EngineView, layers et sous-RenderSnapshots](PR-10-engineview-snapshot.md) | 09 | Construire des snapshots graphiques immuables ordonnés par vues. |
| [PR-11 — Attachments partagés et ressources GPU par Surface](PR-11-gpu-attachments-surfaces.md) | — | Rendre sûrs Texture/Framebuffer versionnés et la récupération GPU en multifenêtres. |
| [PR-12 — EngineWidget et première EngineView visible](PR-12-enginewidget-first-view.md) | 09, 10, 11 | Afficher une vue et sa sortie HDR finale dans un widget sans dessin voxel. |
| [PR-13 — Premier rendu voxel opaque](PR-13-voxel-render-first.md) | 04, 07, 10, 11, 12 | Dessiner une `Voxel::Geometry` texturée avec sa palette dans une EngineView. |
| [PR-14 — retirée](PR-14-assembly-render.md) | — | Conserver le rendu de PR13 et les hiérarchies ECS ; pas de nouvel adaptateur voxel/Model3D. |
| [PR-15 — EngineView comme source de Material](PR-15-material-view-source.md) | 06, 10, 11, 12, 13 | Permettre l'usage de la sortie finale d'une EngineView par un Material dans une autre vue. **(fusionnée #66)** |
| [PR-16 — Skybox unlit à six Materials](PR-16-skybox-component.md) | 06, 10, 11, 12, 15 | Ajouter un Skybox générique comme composant RenderingComponent3D ordinaire. **(fusionnée #67)** |
| [PR-17 — Point, Spot, Directional et scores](PR-17-light-components.md) | 09 | Introduire les sources lumineuses directes et leur mathématique CPU. **(fusionnée #68)** |
| [PR-18 — Light::Channel, Top-N et cohérence CPU](PR-18-light-channel.md) | 09, 10, 17 | Fournir une sélection immuable par EngineView cohérente avec ses mutations. **(fusionnée #69)** |
| [PR-19 — Lighting::System et éclairage Forward](PR-19-forward-lighting.md) | 12, 13, 17, 18 | Éclairer les meshes opaques à partir d'une sélection globale par vue. |
| [PR-20 — ShadowMap::Contributor et MeshFilter](PR-20-shadow-contributor.md) | 04, 09 | Définir des sources d'ombres géométriques explicites indépendantes du visuel. |
| [PR-21 — ShadowResults, admission et durée de vie des surfaces](PR-21-shadowresults-surfaces.md) | 10, 11, 18 | Publier les résultats de shadow maps de façon sûre et isolée par Surface. |
| [PR-22 — ShadowMap::RenderingSystem pour Spot/Directional](PR-22-shadowmaps-spot-directional.md) | 17, 18, 19, 20, 21 | Produire des ombres 2D avec caches depth statiques et finals, puis éclairer avec. |
| [PR-23 — Cubemaps PointLight et PCF inter-faces](PR-23-point-cubemaps.md) | 11, 17, 21, 22 | Étendre les cartes statique/finale aux PointLight omnidirectionnelles. |
| [PR-24 — Sélection et budget PointLight multi-vues](PR-24-point-shadow-budget.md) | 18, 21, 22, 23 | Limiter à chaque image/surface les reconstructions intégrales de cubemaps selon la priorité réelle. |
| [PR-25 — Dessin voxel translucide sur WBOIT/HDR existants](PR-25-wboit-hdr.md) | 11, 12, 13, 19 | Ajouter les draws voxel translucides à la composition EngineView déjà livrée. |
| [PR-26 — Intégration complète et publication v0.1.4](PR-26-integration-validation.md) | 04, 13, 15, 16, 19, 20, 21, 22, 23, 24, 25 | Valider le rendu voxel PR13 avec les nouvelles lumières, ombres et transparences, sans Model3D obligatoire. |

## Règle Git — une branche indépendante par PR

Chaque PR **restante (PR19–PR26)** possède une **branche Git dédiée**, créée depuis la tête courante de `V0.1.4` **au moment de commencer** la fiche, et sa PR GitHub cible exclusivement `V0.1.4`.

1. Pour PR-XX, créer `impl/v0.1.4/XX-nom-de-fiche` **depuis `V0.1.4`**, après fusion dans `V0.1.4` de tous ses prérequis. Ne pas créer les 26 branches en avance.
2. Développer et tester sur cette branche, puis ouvrir une unique PR `impl/v0.1.4/XX-nom-de-fiche` → **`V0.1.4`**. Ne jamais committer l'implémentation directement sur `V0.1.4` ou `main`.
3. Ne pas empiler les branches des fiches dépendantes : fusionner les prérequis dans `V0.1.4` avant de démarrer la suivante. Deux fiches indépendantes peuvent démarrer en parallèle depuis `V0.1.4`, puis se resynchroniser avec sa tête avant fusion.
4. Garder la PR #37 (`V0.1.4` → `main`) comme PR de regroupement de la version. Dès que les PR de code sont fusionnées dans `V0.1.4`, leurs changements apparaîtront aussi dans la #37. Ne fusionner la #37 vers `main` qu'après la validation globale du propriétaire.

## Consignes d'exécution
- Une PR de code par fiche, indépendante lorsque le graphe le permet. Un nom de fiche `PR-XX` ne préjuge pas du numéro réel attribué par GitHub.
- Inspecter les vraies API et les tests, n'introduire aucun moteur de ressources, mutex, cache ou ordonnanceur déjà couvert par Sparkle. `sparkle::core` reste headless ; `sparkle::sparkle` porte le graphique.
- Les signatures illustratives ne sont pas des décisions imposant une refonte. Les détails d'implémentation restent à trancher localement et à tester ; pas de nouveau registre de questions ni de réouverture des choix approuvés.
- Exécuter les tests CTest/GoogleTest pertinents en Debug et Release, Core Windows/Linux et Graphics Windows lorsque ces cibles sont touchées. Les images de référence nécessitent une revue humaine.
- Réaliser une revue indépendante et obtenir accord du propriétaire pour chaque fusion.

**Jalons livrés :** PR01–PR13 (fondations, rendu voxel et EngineView), PR15 (source de vue pour les Materials), PR16 (Skybox et UBO caméra), PR17 (composants lumineux CPU) et PR18 (Top-N par vue). **Jalons restants :** PR19 éclairage Forward, PR20–PR24 ombres, PR25 voxel WBOIT, PR26 intégration. **PR14 reste retirée.**

**Hors scope volontaire :** éviction automatique des caches de géométrie, raffinements adaptatifs d'ombres, Tiled Forward+, moteur de chunks, scheduling GPU global, optimisation SSBO incrémentale.

**Ordre voxel révisé :** PR-01 crée les huit instances dans Shape ; PR-02 (Volume) et PR-03 (clipping + cache facultatif) peuvent ensuite avancer indépendamment ; PR-04 (Mesher + Geometry) attend les deux. Le Mesher sans cache recalcule seulement l'occlusion, jamais les orientations.

## Décisions de référence après PR13–PR18

- Voie de production : `Voxel::Volume` → `Voxel::Mesher` → `Voxel::Geometry` + `spk::Palette` → `Voxel::Renderer` / `Voxel::RenderingSystem` → `EngineView`. Les parents et rotations ECS permettent déjà de composer des personnages et des armes.
- `Model3D::Blueprint`, `ObjectHeadless`, `Object<TGraphics>` restent des API génériques indépendantes de PR08. **Un `Model3D::Blueprint` ne se lie pas directement à `Voxel::Renderer`**.
- Pas de deuxième `Renderer`, `RenderBatch`, pipeline d'instancing, ou WBOIT/HDR dans les PR suivantes : étendre PR13 et la sortie EngineView existante pour éclairage/ombres/translucence.
- La [PR GitHub #64](https://github.com/EreliaStudio/Sparkle/pull/64) est une expérimentation non fusionnée de l'ancien périmètre PR14 et ne constitue pas la baseline approuvée.


## Contrats livrés depuis PR13

- **PR13** conserve `Voxel::Renderer`, `Voxel::RenderingSystem`, `DrawVoxelCommand` et le remapping GPU `MaterialRemap` ; regroupement instancié local par géométrie et palette, sans nouveau gestionnaire de buckets. Les atlas de test golden utilisent `nearest`.
- **PR15** expose `EngineView::Texture` via `view.texture()` et résout la sortie au binding sur la Surface exécutante.
- **PR16** fournit `Skybox` à six Materials et `CameraUniformRenderCommand` : bloc `CameraData`, `std140`, UBO binding 1, partagé avec `DrawVoxelCommand`.
- **PR17–18** fournissent Point, Spot, Directional et `Light::Channel`/Top-N immuable par vue (`N=8`, `h=0,10` initialement ; Directional hors quota).
- **Contrat PR18 accepté :** mutation CPU puis notification. Pas de verrou transactionnel global des setters ; l'application coordonne écritures et captures CPU concurrentes. Le contrôle de génération des Channels reste actif.
