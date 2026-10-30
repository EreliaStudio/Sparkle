# Sparkle v0.1.4 — Dossier de préparation des 26 PR

Ce dossier contient **26 fiches de développement successives**, destinées à une IA implémenteuse. Ce sont des **plans**, pas des PR GitHub déjà créées. Aucun fichier C++ n'est modifié dans ce dossier.

**Références :** [spécification normative](../../voxel-engine-v0.1.4-specification.md) · [PR documentaire #37](https://github.com/EreliaStudio/Sparkle/pull/37) · [dépendances](DEPENDENCIES.md) · [couverture des fonctionnalités](COVERAGE.md) · [prompt pour une IA](AI-HANDOFF.md).

## Fiches

| Fiche | Dépendances minimales (numéros de fiches) | Résultat attendu |
|---|---|---|
| [PR-01 — Shape, Definition et Cell](PR-01-voxel-foundations.md) | — | Constituer le modèle voxel public sans Engine et sans OpenGL. |
| [PR-02 — Volume dense et Builder](PR-02-voxel-volume.md) | 01 | Éditer une grille voxel puis publier des snapshots immuables. |
| [PR-03 — VoxelMesher, clipping et Model](PR-03-voxel-mesher-model.md) | 02 | Générer un modèle voxel complet et déterministe à partir du Volume. |
| [PR-04 — OcclusionCache facultatif](PR-04-occlusion-cache.md) | 03 | Mettre en cache le clipping sans changer les triangles produits. |
| [PR-05 — AssetManager et AssetFactory typés](PR-05-asset-manager.md) | — | Charger et posséder des assets C++ génériques à identité stable. |
| [PR-06 — Material multi-rôles, Texture et SpriteSheet](PR-06-materials-json.md) | 05 | Construire des Materials immuables via Builder ou JSON. |
| [PR-07 — VoxelPalette et compatibilité des Models](PR-07-voxel-palette.md) | 03, 06 | Associer les slots sémantiques de Models à des Materials sans rebake. |
| [PR-08 — Voxel::Assembly par copie et expansion](PR-08-assembly-hierarchy.md) | 07 | Composer une hiérarchie de Parts animables sans Entity par Part. |
| [PR-09 — RenderingEngine : Engine et Contributors](PR-09-rendering-engine-inheritance.md) | — | Moderniser l'API moteur graphique tout en préservant le moteur headless. |
| [PR-10 — EngineView, layers et sous-RenderSnapshots](PR-10-engineview-snapshot.md) | 09 | Construire des snapshots graphiques immuables ordonnés par vues. |
| [PR-11 — Attachments partagés et ressources GPU par Surface](PR-11-gpu-attachments-surfaces.md) | — | Rendre sûrs Texture/Framebuffer versionnés et la récupération GPU en multifenêtres. |
| [PR-12 — EngineWidget et première EngineView visible](PR-12-enginewidget-first-view.md) | 09, 10, 11 | Afficher une vue et sa sortie HDR finale dans un widget sans dessin voxel. |
| [PR-13 — VoxelRenderingSystem et rendu de Model opaque](PR-13-voxel-render-first.md) | 03, 07, 10, 11, 12 | Dessiner un Model voxel texturé dans sa première EngineView. |
| [PR-14 — AssemblyRenderingComponent et instancing articulé](PR-14-assembly-render.md) | 08, 13 | Afficher plusieurs Parts d'Assembly sans rémailler. |
| [PR-15 — EngineView comme source de Material](PR-15-material-view-source.md) | 06, 10, 11, 12, 13 | Permettre l'usage de la sortie finale d'une EngineView par un Material dans une autre vue. |
| [PR-16 — Skybox unlit à six Materials](PR-16-skybox-component.md) | 06, 10, 11, 12, 15 | Ajouter un Skybox générique comme composant RenderingComponent3D ordinaire. |
| [PR-17 — Point, Spot, Directional et scores](PR-17-light-components.md) | 09 | Introduire les sources lumineuses directes et leur mathématique CPU. |
| [PR-18 — Light::Channel, Top-N et cohérence CPU](PR-18-light-channel.md) | 09, 10, 17 | Fournir une sélection immuable par EngineView cohérente avec ses mutations. |
| [PR-19 — Lighting::System et éclairage Forward](PR-19-forward-lighting.md) | 12, 13, 17, 18 | Éclairer les meshes opaques à partir d'une sélection globale par vue. |
| [PR-20 — ShadowMap::Contributor et MeshFilter](PR-20-shadow-contributor.md) | 03, 09 | Définir des sources d'ombres géométriques explicites indépendantes du visuel. |
| [PR-21 — ShadowResults, admission et durée de vie des surfaces](PR-21-shadowresults-surfaces.md) | 10, 11, 18 | Publier les résultats de shadow maps de façon sûre et isolée par Surface. |
| [PR-22 — ShadowMap::RenderingSystem pour Spot/Directional](PR-22-shadowmaps-spot-directional.md) | 17, 18, 19, 20, 21 | Produire des ombres 2D avec caches depth statiques et finals, puis éclairer avec. |
| [PR-23 — Cubemaps PointLight et PCF inter-faces](PR-23-point-cubemaps.md) | 11, 17, 21, 22 | Étendre les cartes statique/finale aux PointLight omnidirectionnelles. |
| [PR-24 — Sélection et budget PointLight multi-vues](PR-24-point-shadow-budget.md) | 18, 21, 22, 23 | Limiter à chaque image/surface les reconstructions intégrales de cubemaps selon la priorité réelle. |
| [PR-25 — WBOIT MRT et composition HDR complète](PR-25-wboit-hdr.md) | 11, 12, 13, 19 | Afficher des surfaces translucides avec une seule passe de géométrie et HDR jusqu'à la sortie. |
| [PR-26 — Intégration complète et publication v0.1.4](PR-26-integration-validation.md) | 04, 08, 14, 15, 16, 19, 20, 21, 22, 23, 24, 25 | Vérifier collectivement toutes les fonctionnalités sans introduire une nouvelle architecture. |

## Règle Git — une branche indépendante par PR

Chaque PR d'implémentation possède une **branche Git dédiée**, créée depuis la tête courante de `V0.1.4` **au moment de commencer** la fiche, et sa PR GitHub cible exclusivement `V0.1.4`.

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

**Jalons :** PR01 premières cellules orientées ; PR03 premier mesh ; PR12 première EngineView visible ; PR13 premier cube voxel texturé ; PR14 Assembly articulée ; PR19 premier éclairage ; PR22 première shadow map ; PR25 transparence ; PR26 intégration complète.

**Hors scope volontaire :** cache par orientation de Shape non approuvé, raffinements adaptatifs d'ombres, Tiled Forward+, moteur de chunks, scheduling GPU global, optimisation SSBO incrémentale.
