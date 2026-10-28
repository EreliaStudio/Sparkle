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


- **Architecture graphique validée :** `spk::RenderingEngine : public spk::Engine`, dans `sparkle::sparkle`, possède les `EngineView` et son `ThreadSafeSlot<RenderSnapshot>`. `Engine` demeure utilisable seul par les serveurs headless et ne dépend ni des vues ni d'OpenGL. `EngineView` n'est plus une classe de `sparkle::core`. Le destructeur d'Engine doit permettre la destruction polymorphique.
- **Caméras et vues explicites :** chaque vue a un nom unique, une caméra, une résolution et un ordre de capture **unique dans son RenderingEngine** ; toute collision à l'ajout ou lors d'un changement d'ordre déclenche une `spk::Exception`. `camera.view(name)` utilise le contexte moteur existant et lance `spk::Exception` sur un nom inconnu ou une mauvaise caméra. `"main"` est uniquement l'identifiant demandé par défaut pour `Drawing`. Aucune vue/caméra/projection/resize automatique.
- **Une classe `EngineWidget` :** `bind(RenderingEngine*)`, non propriétaire ; rôles indépendants et combinables `Update`, `Render`, `Drawing`. Pas d'`EngineViewWidget` distinct et pas de protection des updates/rendus redondants demandés par le développeur.
- **Sous-snapshots par vue (approche A) :** chaque `EngineView` dispose d'un Builder et d'un sous-`RenderSnapshot` indépendants, inséré par commande de délégation dans une passe du snapshot moteur ordonnée par la vue. Une vue est entièrement rendue avant la suivante ; pas de mutualisation inter-vues exigée pour V0.1.4.
- **Contribution explicitement liée à une vue :** `EngineView::Contributor` remplace `RenderingObjectTrait` pour les variantes graphiques de Component/Behaviour/Entity/System. `EngineView::CaptureContext` transmet les informations figées de caméra, matrices, ressources logiques, etc. ; les widgets conservent `RenderSnapshotContributorTrait`.
- **États GPU explicitement préparés par passes :** `RenderingEngine` initialise les passes avec des `RenderCommand` génériques par famille : blending (y compris par attachment), depth, culling, color mask, framebuffer et viewport/scissor adaptés. Chaque phase configure les états qu'elle utilise et la fenêtre restaure explicitement son état de présentation avant Drawing/UI, sans monolithe de sauvegarde d'état GL.
- **Deux snapshots, deux cadences distinctes :** `Engine::updateState` met à jour les entités sans capture. `RenderingEngine::refreshRenderSnapshot` construit et publie le snapshot moteur, uniquement sur demande au moment de préparer un snapshot de fenêtre avec rôle `Render`. L'Application conserve ses slots de snapshots par fenêtre ; une commande y capture et exécute un `shared_ptr<const RenderSnapshot>` moteur. Les fenêtres ne sont pas rafraîchies automatiquement lors de la publication moteur.
- **Ressources OpenGL séparées par fenêtre, sans map dédiée :** `Framebuffer` et `Texture` sont déjà des `GPUResource`, matérialisées par la `GPUResourceCollection` de chaque `Window::Surface`. Une même EngineView peut être rendue dans plusieurs fenêtres ; produire A n'actualise pas B. `Drawing` seul affiche la dernière texture locale courante, sans warning si elle n'a jamais été initialisée.
- **Emprunts et snapshots :** détruire une `EngineView` invalide ses références logiques et les matériaux qui l'empruntent. Les RenderSnapshot déjà publiés peuvent conserver leurs handles GPU concrets jusqu'à utilisation ; ce mécanisme ne prolonge pas l'existence de la vue. Les changements de résolution exigent une nouvelle version de framebuffer/attachments, et non un resize en place des States capturés.

## 2. Décisions structurantes à arbitrer

### P0 — Représentation et ordonnancement complet des vues

**Références :** sections 7 et 7.1 de la spécification.

**Décisions closes.** Approche A : un Builder et un sous-`RenderSnapshot` par EngineView, indépendants et reconstruits à chaque rafraîchissement ; le snapshot moteur possède une commande d'exécution par vue, ordonnée par le jeu. Un même `EngineView::Contributor` remplit le Builder de chaque vue avec son `CaptureContext` explicite. Aucun rendu de vue n'est entrelacé avec le suivant ; pas de mutualisation, de graphe de dépendances ni d'ordonnanceur supplémentaire. Les passes standard sont initialisées avec des RenderCommand d'état GPU et le retour à la présentation précède les widgets UI. **Extension validée (choix B) :** les Contributors peuvent ajouter des passes à ordre numérique explicite via le Builder existant, en respectant les clés et bornes réservées par le moteur ; ces passes restent locales à leur vue. Les slots moteur et fenêtre et les GPUResource par surface restent inchangés.

**Ordre des vues — choix B validé.** Deux vues d'un même moteur ne peuvent pas partager le même ordre. L'ajout ou la modification d'ordre en conflit déclenche une `spk::Exception` descriptive ; le tri stable ne sert pas de départage implicite.

**Questions résiduelles avant implémentation :**

1. **Précision technique restante :** le constructeur ou Builder d'une EngineView exige-t-il toujours un ordre explicite, ou une surcharge sans ordre est-elle proposée ? Toute surcharge doit garantir l'unicité des ordres. Le choix d'interdire les collisions est définitif.
2. **Décision close sur l'extension :** les Contributors ajoutent librement des `RenderPass` locales à leur vue avec des `RenderPass::Key` explicites, à l'intérieur des limites réservées au moteur. **Précision technique restante :** finaliser les noms/ordres exacts, les plages disponibles, ainsi que la validation des collisions et des insertions interdites ; ne pas rouvrir le choix d'un Builder existant sans ordonnanceur supplémentaire.
3. Où précisément placer les commandes de remise en état du framebuffer de présentation dans le **snapshot de fenêtre** pour qu'elles précèdent tous les widgets Drawing/UI, y compris lorsqu'un widget effectue uniquement Drawing ? Les invariants sont validés, leur encodage exact reste à préciser.
4. Confirmer les règles d'erreur des vues manquantes, des changements d'ordre et de l'absence/déconnexion de caméra ; les références pendantes restent sous responsabilité du développeur.

**Critère de clôture.** Deux vues `surveillance` (ordre 0) et `main` (ordre 1) exécutent chacune leur pipeline complet sans collision de nom de passe ; la texture locale de surveillance est consommée par main puis la fenêtre retourne à son état UI correct.

### P0 — Contrat du pipeline Forward, des ombres et du WBOIT

**Références :** sections 7.1 à 7.5.

**Constat.** `Framebuffer`, `RenderPass`, `RenderSnapshot` et les handles GPU existent ; Forward, WBOIT et shadow maps génériques restent à implémenter. Le principe d'état d'entrée explicite par `RenderPass` et les familles génériques de nouvelles `RenderCommand` (Blend, Depth, CullFace, ColorMask, Framebuffer et adaptation Viewport/Scissor) sont validés. Les valeurs de ces états, les formats et le contrat précis du pipeline restent à déterminer. Un suffixe `.init` n'a aucune signification automatique.

**Points restants :**

1. Formats et nombre d'attachments WBOIT (accumulation/révélation), profondeur, **valeurs exactes du blending par attachment**, limites OpenGL et commandes de composition ; `Texture::Format` n'expose pas encore tous les formats potentiellement nécessaires.
2. Paramètres d'éclairage générique (directionnelle, point, spot), limites, shadow maps des directionnelles/spots, qualité, biais, portée et rafraîchissement. Le rendu est préparé indépendamment par vue, **sans mutualisation des shadow maps exigée**. Les points lumineux n'ont pas d'ombres et les directionnelles utilisent une seule carte par vue/lumière ; ni Deferred ni Forward+ n'est prévu.
3. Remplacement sûr des framebuffers/attachments après changement **explicite** de taille : toute capture déjà publiée doit référencer des States immuables, et les anciennes ressources GPU ne doivent pas être réutilisées pendant leur exécution. `Framebuffer::resize()` en place ne fournit pas ce contrat.
4. Comment assurer la **réentrance** des `RenderCommand::execute(RenderContext&) const` d'un snapshot moteur exécuté simultanément dans deux contextes de fenêtre ? Les caches CPU mutables, dont le `ViewportUniformRenderCommand` actuel, et l'état OpenGL par contexte sont à auditer. Ajuster aussi les compteurs et durées du `Profiler`, aujourd'hui calculés par `RenderSnapshot::execute()` de manière plate, pour les sous-snapshots imbriqués.

**Critère de clôture.** Un ensemble de passes par vue, documenté puis testé, marche avec plusieurs vues et deux surfaces GPU, y compris pendant le resize et la coexistence de snapshots anciens et nouveaux.

### P0 — Sources de texture EngineView dans les Materials

**Références :** sections 4.1, 4.4, 7 et 8.1.

**Constat.** Un `Material` statique contient aujourd'hui un `Texture::Handle` exact et immuable. Cela ne suffit pas à une sortie de vue qui change de framebuffer après un resize. Le choix fonctionnel est validé : emprunter l'`EngineView`, capturer le handle concret dans le snapshot, ne jamais prolonger la vie de la vue via le Material. Une source dynamique de vue doit pouvoir alimenter un écran de surveillance.

**Points restants :**

1. Quelle représentation publique (variante de source, overload de Builder, autre type minimal) porte une référence **non propriétaire** à une EngineView et permet de la résoudre pendant `refreshRenderSnapshot()` sans nouvelle lecture pendant le rendu GPU ?
2. Comment cette source s'intègre-t-elle à la **règle d'un seul atlas par passe et par couple Model/Palette**, actuellement fondée sur des identités `Texture::Handle` stables ? Une vue peut-elle fournir le rôle `albedo` d'un Material voxel ? Si oui, quel est le contrat de compatibilité avant le premier rendu et après resize ?
3. **Résolu pour l'ordonnancement :** la vue A est entièrement exécutée avant B dans la même surface grâce aux sous-snapshots séquentiels, sans transfert interfenêtre ni dépendance déduite du Material. Reste à préciser le binding concret de la texture de A dans les commandes de B et le traitement d'une référence invalide ou d'une dépendance cyclique déclarée par le jeu.
4. Quelles opérations sur une vue (renommage, suppression, rebinding de caméra, remplacement de cible) sont autorisées, et comment leurs références empruntées deviennent-elles invalides ou restent-elles stables ?

**Critère de clôture.** Un écran 3D de vidéosurveillance reste affichable après resize explicite sans reconstruire son Material, tandis qu'un Material référençant une vue détruite n'est plus utilisable.

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

**Ce qui est acquis.** Le snapshot moteur est publié comme une valeur immuable ; ses commandes n'empruntent aucun objet mutable du jeu. Le slot supporte de multiples lecteurs et les ressources GPU sont matérialisées par surface. Les éléments de l'Engine sont mis à jour régulièrement tandis que le snapshot n'est construit **que sur demande d'une fenêtre avec rôle Render**.

**Points restants :**

1. Sur quel thread et pendant quelles frontières de cycle sont autorisées les mutations de `Engine`, `EngineView`, caméra, lumières, Assemblies et composants, face à `refreshRenderSnapshot()` ?
2. Les `Model*`, `VoxelPalette*` et `Assembly*` empruntés doivent rester valides pendant la préparation des commandes : quel est le contrat explicite de destruction/détachement ?
3. Les deux fenêtres peuvent demander une capture à des moments distincts dans le même cycle Update. Faut-il garantir un snapshot logique commun, ou accepter deux captures cohérentes mais datées différemment (l'absence de déduplication est déjà validée) ?
4. Comment la distribution des événements et `DeviceContext` de l'ancien RenderingEngine adaptateur est-elle conservée avec l'héritage, notamment avec des widgets `Render|Drawing` sans `Update` et plusieurs fenêtres à focus différent ?
5. Quelles ressources sont figées en handles et en copies (meshes, matrices, lumières, framebuffer, Material, SSBO), et quel usage partagé reste interdit pendant `execute()` ?

**Critère de clôture.** Une séquence complète « update logique → capture CPU → publication → exécution parallèle dans A et B » garantit l'absence de lectures mutables et de races dans les composants Sparkle (hors erreurs explicitement laissées au développeur).

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

1. **P0 partiellement clos :** les sous-snapshots par vue, `EngineView::Contributor`, les commandes d'état typées et l'extension par passes personnalisées explicites sont validés ; finaliser les valeurs des clés/plages réservées, la politique éventuelle d'ordre par défaut et le placement exact des commandes de présentation UI.
2. **P0 :** fixer les configurations concrètes Forward/ombres/WBOIT (formats, blending par attachment et composition) et le remplacement des cibles lors des resizes en protégeant tous les snapshots en vol.
3. **P0 :** finaliser la source `Material` fondée sur une `EngineView` et l'identité d'atlas dans les `VoxelPalette`, pour que la vidéosurveillance soit réellement implémentable.
4. **P1 :** clarifier les frontières de thread et de propriété pendant la capture, y compris input et lifetimes des composants, puis la composition des Assemblies et le JSON complet des Materials.
5. **Préparation technique :** finaliser tests, algorithmes géométriques, layouts GPU et APIs dans une seule passe cohérente ; seules les implications architecturales demandent un nouveau choix du mainteneur.

Les décisions validées résident uniquement dans la spécification principale. Cette liste doit décroître au fur et à mesure que leurs contrats techniques sont vérifiés ; elle ne vaut pas autorisation d'implémentation.
