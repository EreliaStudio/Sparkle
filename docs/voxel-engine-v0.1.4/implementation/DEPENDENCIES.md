# Graphe de dépendances et chemin critique

Les liens entre fiches sont des prérequis **techniques**, pas seulement un ordre de numérotation.

```mermaid
flowchart TD
  P01["01 Shape"] --> P02["02 Volume"] --> P03["03 Mesher"]
  P03 --> P04["04 OcclusionCache"]
  P05["05 Assets"] --> P06["06 Material"]
  P03 --> P07["07 Palette"]
  P06 --> P07 --> P08["08 Assembly"]
  P09["09 RenderingEngine"] --> P10["10 EngineView"]
  P11["11 GPU Targets"] --> P12["12 EngineWidget"]
  P10 --> P12
  P09 --> P12
  P03 --> P13["13 Voxel opaque"]
  P07 --> P13
  P12 --> P13 --> P14["14 Assembly Render"]
  P08 --> P14
  P06 --> P15["15 Material source View"]
  P12 --> P15
  P13 --> P15 --> P16["16 Skybox"]
  P06 --> P16
  P09 --> P17["17 Lights"]
  P17 --> P18["18 Channel Top N"]
  P10 --> P18
  P18 --> P19["19 Forward"]
  P13 --> P19
  P17 --> P19
  P03 --> P20["20 Shadow Contrib"]
  P09 --> P20
  P18 --> P21["21 ShadowResults"]
  P11 --> P21
  P19 --> P22["22 Spot Directional Shadows"]
  P20 --> P22
  P21 --> P22 --> P23["23 Point Cubemaps"] --> P24["24 Point Budget"]
  P18 --> P24
  P19 --> P25["25 WBOIT"]
  P11 --> P25
  P13 --> P25
  P04 --> P26["26 Integration"]
  P08 --> P26
  P14 --> P26
  P15 --> P26
  P16 --> P26
  P24 --> P26
  P25 --> P26
```

**Deux chaînes parallèles au début :** la chaîne voxel (PR01–04) et la chaîne graphique (PR09–12). La chaîne AssetManager/Material (PR05–06) peut commencer sans les autres. Elles convergent sur PR13. Les ombres dépendent de l'éclairage mais PR25 WBOIT ne dépend pas du calcul des shadow maps ; elle peut progresser en parallèle des PR20–24.

**Chemin critique visible :** PR09 → PR10 et PR11 → PR12, puis PR03 + PR07 + PR12 → PR13. Il donne le fond d'une EngineView d'abord, puis un vrai voxel texturé.

**Risques :** préserver l'input après le remplacement de RenderingEngine en PR09, l'immuabilité des anciens Handles au resize en PR11 et les transactions mutation+invalidation en PR18. Tester ces frontières dès les PR correspondantes.

**Mode de collaboration :** fusionner d'abord les prérequis nécessaires, éviter les chaînes de branches divergentes et toute PR géante ; il n'est pas nécessaire de linéariser des branches indépendantes.
