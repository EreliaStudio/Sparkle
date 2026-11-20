# PR08 — Voxel::ChunkMesher et adaptateur graphique des chunks
**Couche :** Graphics seulement. **Dépendances :** PR00, PR02, PR07. **État :** cahier des charges.

## Contrat — meshing des voisins
- Introduire spk::Voxel::ChunkMesher dérivant de l'actuel spk::Voxel::Mesher. Réutiliser bake(const Volume&) et l'algorithme d'occlusion sans modification.
- Redéfinir uniquement outsideCell(position,face) afin d'interroger le Volume du chunk voisin pour les six faces. Respecter les coordonnées locales, dimensions, orientions et les états OutsideCell::State. Chunk inexistant => comportement Empty compatible avec le Mesher actuel.
- Résoudre les six voisins à **chaque cuisson** depuis la collection (ou six références de très courte durée fournies lors de cette cuisson), sans stocker les voisins dans Voxel::Chunk et sans nouveau MesherContext obligatoire. Toute référence n'est valide que pendant le bake.

## Contrat — adaptation graphique
- RenderingSystem ou adaptateur explicite lié à ChunkCollection, actif uniquement avec Engine graphique ; sur bind, scanner les chunks existants.
- S'abonner au **seul** événement coordonné de la collection ; le callback place des coordonnées dans un set de travail sale, et le système synchronise au plus une fois chaque coordonnée avant le rendu. Aucun traitement GPU directement pendant le setter du chunk.
- Ajout/modification/remplacement d'un chunk : déclarer systématiquement sales **le chunk et ses six voisins**, sans comparer les frontières. Sur disparition : écouter la destruction LifeCycle de l'Entity3D, libérer les ressources graphiques externes, invalider les six voisins ; ne jamais lire une entité partiellement détruite.
- Conserver l'Entity3D headless et ses composants existants lors d'une édition ou remplacement. Ajouter/réutiliser Voxel::Renderer et la Geometry/Palette suivant les APIs réelles ; aucun lien magique Volume→Renderer.
- Les références et abonnements sont nettoyés lors d'un unbind ou changement d'Engine, jamais reconnectés implicitement. Aucune obligation d'un mesh unique par chunk.

## Tests et golden images
Baking avec six voisins et sans voisin, faces aux coordonnées négatives, jonction de voxels opaques et translucides, fenêtres et formes variées, arrivée/départ de voisins, édition rapide multiple coalescée en une seule reconstruction par frame, rendu avec caméra inclinée, ressources détruites sans use-after-free, EngineHeadless sans création OpenGL.

## Exclusions
Aucune modification des algorithmes de Voxel::Mesher, aucun cache voisin dans les chunks, aucune nouvelle dépendance Graphics dans Core et aucune optimisation par détection de faces modifiées.
