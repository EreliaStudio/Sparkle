# Sparkle V0.1.4 — Questions ouvertes pour finaliser l'architecture

> **Document de travail complémentaire** à la [spécification d'architecture cible](voxel-engine-v0.1.4-specification.md).
>
> **But :** identifier uniquement les décisions et les précisions nécessaires pour transformer la spécification en un contrat de développement réalisable, sans faire choisir au mainteneur chaque détail technique. Ce document n'est **ni un historique de discussion, ni une liste de tâches d'implémentation, ni une nouvelle source de vérité concurrente** : lorsqu'un point est résolu, la règle retenue doit être intégrée dans la spécification principale et le point retiré ou simplifié ici.
>
> **Statut :** questions issues d'une lecture de la spécification comme document autonome et d'une confrontation ciblée avec les API existantes de Sparkle sur la branche `V0.1.4`. Aucun des choix proposés ci-dessous n'est réputé validé sans décision explicite du mainteneur. Aucun code n'est autorisé par ce document.

## 1. Contexte confirmé : ce que la revue ne doit pas remettre en question

La spécification principale établit déjà les éléments suivants, qui ne sont **pas** à rouvrir à l'occasion de cette liste :

- Les Shapes et Definitions immuables, Cells par valeur, Volume dense à table de Definitions, bake synchrone et complet, deux meshes (opaque/translucide), culling par géométrie/opacité des slots, et cache d'occlusion directionnel facultatif.
- Les Materials génériques, les VoxelPalettes immuables, la sélection de sprites par `SpriteSheet`, la limitation à un atlas `albedo` par passe et par couple Model/Palette, et la séparation apparence/géométrie.
- La composition de Model dans un Assembly à parties articulées, et un VoxelRenderingSystem qui collecte les composants et reconstruit les SSBO de chaque snapshot sans cache inter-frame.
- Les factories statiques par type de l'AssetManager, la clé `(T, id, JSON structurel)`, les retours `T&` empruntés, les constructions `T` par valeur, la synchronisation externe, la détection des cycles et la purge explicite `clear()`.
- **Confirmation du mainteneur : Sparkle ne dispose pas encore d'un système d'éclairage ou d'ombres. Il faut créer ces fonctionnalités dans V0.1.4.** La branche actuelle ne contient pas non plus de pipeline WBOIT complet identifié durant la revue. Les briques graphiques disponibles (`Program`, `Framebuffer`, `RenderPass`, `RenderSnapshot`, `Texture`, `ShaderStorageBuffer`, etc.) sont des fondations, **pas** un pipeline 3D complet à réutiliser tel quel.
- L'éclairage, les ombres et la transparence ne doivent pas devenir des services exclusivement voxels : l'objectif reste une bibliothèque Sparkle publique et générique.
- **Le pipeline générique 3D utilise le Forward Rendering en V0.1.4 :** l'éclairage est calculé pendant le dessin des surfaces, pour l'opaque comme pour le translucide avant accumulation WBOIT. Ni rendu Deferred avec G-buffer d'éclairage, ni organisation Forward+ des lumières par tiles/clusters ne sont requis. Les passes auxiliaires de profondeur, shadow maps et WBOIT restent possibles.
- **L'ordonnancement existant est conservé :** `RenderingEngine` et les `RenderSnapshot`/`RenderPass`/`RenderCommand` orchestrent les nouvelles étapes 3D. Si une passe nécessite une initialisation distincte, une paire de passes nommées par convention `renderPassA.init` puis `renderPassA` est possible, avec `init.order < draw.order` **explicitement défini**. Le nom ne crée pas de comportement automatique. Aucun nouvel ordonnanceur ou `SceneRenderingSystem` central n'est exigé.

- **Vues 3D nommées dans `sparkle::core` :** l'Engine possède des `spk::EngineView`, indexées par identifiant string, chacune associée explicitement à une caméra de cet Engine et à une résolution demandée. Les `EngineView` et `CameraComponent` ne possèdent aucun `Framebuffer` ni ressource OpenGL. `"main"` est seulement le `ViewID` affiché par défaut par `EngineWidget` ; la vue et sa caméra doivent être créées explicitement par l'application. `EngineWidget` met à jour l'Engine et affiche sa vue sélectionnée ; `EngineViewWidget` affiche une vue nommée sans mettre à jour l'Engine.
- **Cache graphique partagé par fenêtre :** dans `sparkle::sparkle`, les ressources de rendu persistent et sont indexées par `(Window::Surface, Engine, ViewID)`, sans duplication du rendu d'une même vue au sein d'un snapshot de fenêtre. Les captures utilisent les `RenderPass` ordonnées ; l'image produite est réutilisable par plusieurs widgets ou objets sans nouveau calcul.
- **Aucun resize implicite :** seul le code du jeu décide de modifier la taille d'une `EngineView` et, indépendamment, les paramètres de projection de sa caméra. Le resize d'une fenêtre ou d'un widget ne modifie jamais ces valeurs ; le cache graphique ajuste ses ressources uniquement après changement explicite de la résolution de l'EngineView. Un widget peut afficher une texture à une autre taille sans redimensionner la capture.

## 2. Décisions structurantes à arbitrer

### P0 — Architecture du rendu 3D générique (priorité la plus élevée)

**Référence dans la spécification :** sections 1, 7.1 et 7.5.

**Situation.** Le Forward Rendering, les `CameraComponent` et les trois types de lumières 3D, les ombres directionnelles et spots, le choix d'une carte unique par lumière directionnelle (sans CSM), ainsi que l'ordonnancement par `RenderPass` sont déjà validés. Le modèle de **`spk::EngineView` nommées et indépendantes d'OpenGL**, la séparation `EngineWidget` / `EngineViewWidget`, et le cache par `(Window::Surface, Engine, ViewID)` sont également **actés** dans la spécification principale. Les ombres sont activées par défaut pour les directionnelles et les spots, avec possibilité de les désactiver ou réactiver. Ces principes ne doivent plus être rediscutés. L'éclairage, les ombres et le WBOIT sont à développer en V0.1.4, de façon générique et non réservée au voxel.

**Questions architecturales restantes :**

1. Quelles sont les techniques de génération et politiques de qualité des **ombres directionnelles et spots** : projection/cadrage, résolution, biais, rafraîchissement, effets de plusieurs vues et réglages ? Une seule shadow map par directionnelle et par rendu est acquise ; la politique exacte de partage éventuel des ombres entre vues reste à définir.
2. Quels types de **projection de caméra** sont proposés ? Quelles règles d'intégrité et d'API assurent qu'une `EngineView` référence une caméra valide appartenant à son Engine et que `"main"` ou tout autre `ViewID` absent ne déclenche pas de caméra de secours implicite ? Les redimensionnements et projections **restent sous le contrôle explicite du jeu**.
3. Quelles contributions et commandes GPU communes préparent les données de lumières, les shadow maps et les cibles WBOIT, avec les `RenderingEngine` et `RenderSnapshot` déjà existants, et comment les systèmes voxel et non voxel y participent-ils sans pipeline séparé ?
4. Quelle interface publique définit les vues nommées et leurs modifications (enregistrement, retrait, taille, caméra), sans imposer de signatures ni de politiques de durée de vie implicites ?

**Contrat acquis.** La vue logique est propriétaire de son identifiant, de sa caméra associée et de sa résolution, **pas** de ressources GPU. Le cache graphique par fenêtre est le propriétaire des ressources persistantes. `RenderingEngine` prépare les captures des vues enregistrées une seule fois par snapshot de fenêtre ; les widgets consomment les textures obtenues. Aucun nouveau gestionnaire de passes n'est nécessaire. Une `EngineView` ou une caméra n'est pas créée automatiquement pour `"main"`.

**Critère de clôture.** Les projections et règles de validité des références sont définies, les paramètres des trois types de lumières et les techniques d'ombres sont suffisamment précis, et les interfaces graphiques communes sont décrites sans contredire la séparation `sparkle::core` / `sparkle::sparkle`.

### P0 — Orchestration des passes, depth, shadow maps et WBOIT

**Référence :** sections 7.1 à 7.5.

**Situation.** Les passes du `RenderSnapshot` sont ordonnées par `RenderPass::Key::order`. Une passe d'initialisation distincte (convention de suffixe `.init`) peut précéder explicitement une passe de dessin. Les vues nommées sont déclarées côté Engine, leurs cibles persistent dans un **cache graphique par fenêtre**, indexé par `(Engine, ViewID)`, et une capture ne doit pas être répétée par plusieurs widgets de la même fenêtre. **Le dimensionnement des cibles dépend exclusivement des changements explicites de résolution des `EngineView` par le jeu, jamais de la taille des widgets ou fenêtres.** Les formats/attachments nécessaires au rendu WBOIT ne sont pas encore tous disponibles. Les nouvelles commandes graphiques restent à concevoir.

**Questions architecturales restantes :**

1. Quels noms et ordres numériques de passes seront utilisés pour chaque **EngineView** (ombres, opaque/profondeur, accumulation et révélation WBOIT, composition), et comment éviter les collisions entre plusieurs vues et avec les passes d'UI ?
2. Comment implémenter le cache graphique partagé au niveau **fenêtre** en respectant les threads update/render et les `GPUResource::Handle` ? Comment garantir qu'un `Framebuffer` remplacé à la suite d'une modification **explicite** de taille reste valide pour les anciens snapshots, et que des fenêtres distinctes ne partagent pas implicitement leurs contextes ?
3. Comment les états OpenGL de cible, profondeur, blending, culling et texture sont-ils définis et restaurés pour chaque passe ? Quels formats et attachments manquent pour le WBOIT, et comment protéger les autres widgets et objets 3D contre les états résiduels ?
4. Comment adapter le `RenderingEngine` **actuellement détenu par chaque `EngineWidget`** afin qu'un snapshot de fenêtre prépare les captures des vues **une fois**, y compris pour plusieurs widgets montrant la même vue, sans créer un second ordonnanceur ? Séparément, comment empêcher plusieurs `EngineWidget` de mettre à jour plusieurs fois le même Engine pendant une frame ?

**Contrat acquis.** L'Engine est responsable des `EngineView` logiques ; le cache graphique **par surface de fenêtre** possède les cibles persistantes. `EngineWidget` met à jour l'Engine et affiche la vue `"main"` par défaut (ou son `ViewID` configuré). `EngineViewWidget` n'effectue que l'affichage d'une vue choisie. Les commandes de capture s'exécutent avant les commandes de présentation dans les `RenderPass` existantes ; la lecture d'une vue depuis un widget ne provoque **aucun rendu supplémentaire**. Aucun changement de taille/projection n'est déclenché implicitement par Sparkle.

**Critère de clôture.** Les formats GPU, la propriété du cache, la synchronisation entre snapshots, les commandes et les ordres de passes permettent de rendre chaque vue une fois puis d'afficher sa texture dans un ou plusieurs widgets, même lorsque leurs tailles divergent de celle de la vue.

### P1 — Composition d'Assembly dans un autre Assembly

**Référence :** sections 6.1 et 8.1.

**Situation.** La spécification dit qu'une partie peut servir de point d'attache à un `Model` ou à un sous-assemblage, mais l'objet `Part` est surtout décrit comme portant `Model + Palette + transform + parent`.

**Question à arbitrer.** Quand on attache une Assembly B dans une Assembly A, que devient B ?

- **Approche 1 :** les parties de B sont copiées/importées dans la hiérarchie de A ; A garde une seule hiérarchie, sans référence vivante à B.
- **Approche 2 :** une Part de A contient/référence réellement B, avec des règles explicites de propriété, copie, édition, transformation et interdiction des cycles entre Assemblies.

**Conséquences.** Ces modèles diffèrent pour l'identité des PartID, l'animation, le détachement, la copie autonome, la composition récursive des matrices et le partage éventuel de sous-assemblages.

**Direction recommandée, non validée.** Privilégier **une seule hiérarchie de parties** et une importation explicite si cela suffit aux usages prévus ; éviter une deuxième structure récursive sans bénéfice concret.

**Critère de clôture.** Un exemple « personnage → bras → arme » montre l'ajout, l'animation, la copie et le détachement avec leur comportement exact.

### P1 — Forme publique complète des Materials JSON et de leur chargement

**Référence :** sections 4.1 à 4.4 et 5.2.

**Situation.** Les blocs JSON `Texture`/`UV` sont détaillés, mais ils ne décrivent **pas** un Material complet contenant plusieurs **rôles de textures nommés**. La factory générique des assets et le Builder de Material doivent se rencontrer à cette frontière.

**Questions à arbitrer :**

1. Quelle structure JSON représente **l'ensemble des rôles** (`albedo` et rôles applicatifs libres), chacun avec sa texture et sa sélection UV ? Quelles sont les règles de validation des doublons et des rôles manquants ?
2. Comment choisir explicitement entre `Texture` simple et `SpriteSheet`, et à quel niveau `AssetManager::load<T>(id, config)` est-il appelé ? La **configuration JSON de l'asset** doit rester distincte de la **région UV propre au rôle dans le Material**.
3. Quels formats de construction sont exposés publiquement : `Material::Builder` alimenté manuellement, parsing JSON lié à un AssetManager, et éventuelle spécialisation `AssetFactory<Material>` ? Ne pas créer deux chemins divergents vers la même valeur finale.

**Direction recommandée, non validée.** Un dictionnaire explicite de rôles de texture. Chaque entrée fournit la description de l'asset et éventuellement sa région UV ; le résultat compilé dans Material est toujours `Texture::Handle + Texture::Section`.

**Critère de clôture.** Un **JSON complet**, compréhensible sans explication externe, charge un Material contenant au moins deux rôles ; un second exemple sélectionne un sprite par coordonnées. Les mêmes valeurs sont constructibles en C++.

### P1 — Contrat de synchronisation entre simulation, composants et snapshots

**Référence :** sections 6.1, 6.2, 7.1 et 8.1.

**Situation.** L'Assembly est mutable ; les composants empruntent des pointeurs ; le système de rendu construit un snapshot qui ne doit plus lire ces objets ensuite. Sparkle possède plusieurs contextes d'exécution. Le document ne précise pas suffisamment **qui peut modifier/détruire quoi pendant la collecte du snapshot**.

**Questions à arbitrer :**

1. Dans quel contexte/thread s'effectue la **lecture** des composants, des transformés d'Entity et des Assemblies ? Quelles sont les exigences de synchronisation face aux modifications de simulation ?
2. À quel moment les références non propriétaires (`Assembly*`, `Model*`, `VoxelPalette*`) peuvent-elles être détachées ou détruites, sans être observées pendant une collecte ?
3. Quelles données sont copiées ou capturées par le snapshot (matrices, handles, meshes, SSBO), et quelles données restent-elles interdites d'accès depuis les RenderCommand différées ?
4. Une modification en plein build est-elle exclue par un contrat de threading existant, ou un mécanisme explicite est-il nécessaire ?

**Direction recommandée, non validée.** Utiliser les frontières de snapshot déjà présentes dans Sparkle, éviter toute référence vers la hiérarchie mutable dans une commande, et faire porter à l'application un contrat de vie clair pour les pointeurs empruntés.

**Critère de clôture.** Une séquence « update → construction snapshot → rendu » fixe le point à partir duquel les données GPU sont autonomes et les emprunts ne sont plus nécessaires.

## 3. Précisions techniques importantes à mener pendant la préparation de l'implémentation

Ces sujets nécessitent des contrats vérifiables, mais **pas nécessairement de nouvelles décisions du mainteneur** : l'IA de conception peut proposer des détails qui respectent les invariants déjà établis, puis ne soumettre que les conséquences architecturales éventuelles.

### Géométrie robuste et cache d'occlusion

La spécification impose un clipping flottant de polygones plans convexes, une triangulation déterministe des n-gones à UV arbitraires, l'union de la couverture occultante, la suppression des fragments dégénérés et une tolérance `1e-5` configurable. Avant de coder :

- préciser les tests de **coplanarité**, les seuils géométriques dimensionnellement cohérents, la détermination d'une **fermeture complète** et le traitement des frontières quasi confondues ;
- préciser l'interpolation UV/normales quand un fragment traverse plusieurs triangles sources ;
- démontrer que clipping et cache produisent la **même géométrie observable**, et qu'aucun rejet numérique ne ferme artificiellement une face ;
- détailler la forme des **clés directionnelles** du cache et la comparaison des données de Shape, des orientations et des opacités ;
- vérifier que rotation, flip, winding CCW et occlusion frontale donnent des résultats cohérents dans tous les cas.

**Risque si rien n'est précisé :** chaque développeur pourrait choisir des tolérances et une triangulation incompatibles avec le rendu souhaité. Cela justifie un plan d'algorithmes et des exemples géométriques, pas de rouvrir tous les contrats des Shapes.

### Intégration réelle de Mesh, GPUResource et durées de vie

`Model` réutilise `spk::Mesh<TVertex>`, dont les `LayoutBuffer` font partie de l'infrastructure graphique existante. Les données sources `Shape`, `Definition`, `Cell` et `Volume` restent indépendantes d'OpenGL, mais il faut vérifier :

- si un bake lancé hors thread de rendu peut préparer un `spk::Mesh` sans disposer d'un contexte OpenGL courant, la création effective des objets GPU étant différée ;
- comment `DrawVoxelBucketCommand` retient les meshes, `Texture::Handle`, SSBO et autres ressources jusqu'à l'exécution et jusqu'à la fin de leur usage effectif ;
- les bindings de sampler et de SSBO, la structure réelle de `MeshVertex` et le layout, sans inventer une seconde abstraction de buffers ;
- la comparaison des `Texture::Handle` pour établir l'unicité d'atlas par passe : le type actuel fournit `identifier()` et `generation()`, mais pas nécessairement un opérateur `==` public ; retenir une **identité de State**, pas une égalité par contenu d'image.

**Risque si rien n'est précisé :** le rendu fonctionne en test simple mais échoue avec deux snapshots en vol ou des modèles créés sur un thread de travail.

### APIs finales et erreurs

Les principales responsabilités sont établies ; plusieurs noms/types exacts restent délibérément indicatifs. Avant livraison du contrat de développement, finaliser dans une **seule passe d'interface** :

- les méthodes de construction de `Shape::Builder`, de `Definition`, de `Volume::Builder` et de `VoxelPalette::Builder`, avec la garantie forte d'exception et la consommation après succès ;
- la spécialisation `AssetFactory<T>` et ses contraintes C++23, notamment le cas d'un `T` non copiable/non déplaçable et une absence de factory ;
- les noms/types des slots, orientations, `PartID` et transforms locaux d'Assembly ;
- les liaisons des textures nommées du Material et les variantes de construction JSON ;
- les erreurs `spk::Exception` liées à la géométrie, aux associations Model/Palette, aux dépendances circulaires et aux appels invalides.

**Risque si rien n'est précisé :** la cohérence du concept subsiste, mais différentes IA produiraient des APIs publiques divergentes. Il faut obtenir une **interface compilable consolidée**, sans transformer chaque choix de nom mineur en arbitrage architectural.

## 4. Ordre de résolution recommandé

1. **Pipeline graphique 3D générique**, puis **organisation des passes GPU** : ce bloc est le plus dépendant de capacités encore inexistantes dans Sparkle.
2. **Composition des Assemblies** et **frontière de synchronisation des snapshots** : ces décisions affectent la propriété et la validité des données.
3. **JSON Material complet** : stabiliser la forme publique de construction/chargement.
4. **Préparation technique** du meshing, de l'intégration GPU et des signatures finales : produire des propositions et exemples de tests sans multiplier les questions du mainteneur.
5. Reporter les décisions validées dans la **spécification d'architecture cible**, puis retirer les questions correspondantes de ce fichier jusqu'à ce qu'il ne reste aucun **point bloquant**.

Ce document peut rester présent tant que l'architecture est encore discutée, puis être supprimé lorsque la spécification principale devient suffisamment autonome pour guider le développement.
