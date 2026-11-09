# PR-10 — EngineView, layers et sous-RenderSnapshots

> **PR GitHub [#59](https://github.com/EreliaStudio/Sparkle/pull/59) en brouillon ; implémentation en cours de validation CI.** Ne pas fusionner sans approbation explicite.
>
> **Branche source :** `impl/v0.1.4/10-engineview-snapshot`, créée depuis `V0.1.4` au commit `20e317535ce99a629e6c89a1323c579c7b009dd1`. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Construire des snapshots graphiques immuables ordonnés par vues.

**Prérequis :** [PR-09](PR-09-rendering-engine-inheritance.md)

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [x] EngineView nom unique et immutable
- [x] CameraComponent non propriétaire/détachable
- [x] ordre de capture
- [x] layers
- [x] CaptureContext
- [x] Builder de sous-snapshot
- [x] cinq passes réservées
- [x] ThreadSafeSlot moteur
- [x] suppression de vue.

## Instructions techniques et intégration

UpdateState séparé de refreshRenderSnapshot; tous jeux de passes construits CPU sans GL; filtres non configuré=tous et configuré vide=aucun; rejeter clés personnalisées invalides; autoriser snapshot historique cohérent après mutation, sauf removeView avant commit; valeurs/handles capturés sans relecture à execute.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [x] Deux vues mêmes contributeurs avec layers distincts
- [x] 5 clés système et collision nom/ordre
- [x] vues ordonnées/non-ordonnées
- [x] changement de caméra
- [x] suppression/recréation nom
- [x] publication échouée conserve précédent
- [x] snapshot T1 publié après mutation T2.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Sous-snapshots lisibles et exécutables via RenderCommand d'essai, sans contexte GL actif pendant capture.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

ordonnanceur de vues, EngineViewWidget, ouverture implicite de vue main.

## Consigne de passation

Confier **uniquement PR-10** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).

## Implémentation de la PR GitHub #59

- `Engine::addView(name, CameraComponent3D*, resolution, optionalOrder)` crée explicitement et possède la vue ; `view(name)`, `removeView(name)` et `EngineView::setOrder(optionalOrder)` contrôlent nom, incarnation et ordre. Aucune vue `main` implicite.
- `EngineView` expose caméra non propriétaire, résolution, layers optionnels, couleur RGB, exposition, objectif FPS et `ambientLight()` donnant une référence directe à une ambiance **possédée par chaque vue**. `AmbientLight` démarre avec RGB blanc et intensité zéro ; le contexte capture la contribution HDR par valeur sans la limiter à 1.
- `CaptureContext` est une copie CPU des matrices, de l'identité de vue, des paramètres et du filtre ; `isLayerCompatible` admet tout lorsqu'aucun filtre n'est configuré, rien pour `setLayers({})` et l'intersection non vide autrement. Aucun layer n'est hérité automatiquement entre entités et composants.
- Les composants, comportements, entités et systèmes graphiques dérivent du **même** contrat `EngineView::Contributor`. L'en-tête historique `RenderingObjectTrait` reste un simple alias de compatibilité, pas un second mécanisme. `RenderSnapshotContributorTrait` reste disponible pour les widgets.
- `refreshRenderSnapshot()` est indépendant d'`updateState()`, construit cinq passes système par vue active sur CPU, valide les autres clés, puis publie **une seule fois** via `ThreadSafeSlot<RenderSnapshot>`. Les commandes de délégation détiennent un `shared_ptr<const RenderSnapshot>` ; un échec préserve la publication antérieure. Le commit vérifie les identités des vues contre les suppressions, y compris les homonymes recréés, sans rejeter les mutations ordinaires.
- Le développeur possède la durée de vie de `CameraComponent3D*` et doit retirer les références de vues avant de détruire la caméra. Une référence `EngineView&` obtenue par `view()` devient invalide immédiatement après `removeView()`. Les Contributors doivent encoder **des valeurs et handles immuables** dans les commandes, jamais relire leurs objets sources à l'exécution.
- Les sorties GPU, les attachments versionnés, le placement effectif de l'EngineWidget, le pipeline voxel/WBOIT et l'éclairage direct restent réservés aux PR ultérieures. La PR10 fournit les **phases logiques** sans simulation d'un pipeline GL encore absent.

**Validations :** les tests PR10 ont été ajoutés par fonctionnalité. Conserver les cases de critères de fusion ouvertes jusqu'au résultat des builds Debug/Release, des tests Core/Graphics et des golden tests Windows.
