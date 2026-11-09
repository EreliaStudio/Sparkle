# PR-11 — Attachments partagés et ressources GPU par Surface

> **PR en cours :** [#60 — Shared GPU attachments and per-Surface resource lifecycle](https://github.com/EreliaStudio/Sparkle/pull/60), ouverte en brouillon. Ne pas fusionner sans validation du propriétaire.
>
> **Branche source :** `impl/v0.1.4/11-gpu-attachments-surfaces`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Rendre sûrs Texture/Framebuffer versionnés et la récupération GPU en multifenêtres.

**Prérequis :** Aucune PR d'implémentation requise.

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [x] Texture render-target 2D et profondeur cubemap : description CPU et matérialisation différée
- [x] Formats RGBA16F, SRGB8_ALPHA8 et Depth32F ; données CPU RGBA16F refusées
- [x] Framebuffer fondé sur des Texture::Handle explicitement partageables
- [x] Framebuffer depth-only et sélection de la face cubemap lors du binding GPU
- [x] RenderToFramebufferCommand imbriquée : target versionnée, viewport, restore exception-safe et clear via commandes
- [x] Resize historique non destructif ; refus des resizes incompatibles sur les attachments partagés
- [x] Abonnement RAII à la fermeture Surface, notification synchrone et nettoyage dans l'ordre
- [x] Reclaim par tick de Surface Ready avec makeCurrent explicite, même sans consommateur
- [ ] CI Windows/Graphics et clang-format verts sur le commit final.

## Instructions techniques et intégration

Réutiliser GPUResourceCollection et ses pools, aucun nouveau manager; un State capturé ne change jamais lors du resize; `Framebuffer::resize` sur partage explicite refuse; matérialiser par contexte; makeCurrent avant reclaimReleased chaque tick Ready même sans redraw; notifier fermeture avant GPU clear puis WGL.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [x] Deux FBO ayant exactement le même depth handle et la même texture native sur une Surface
- [x] Anciens States valides après resize, dimensions et identités conservées
- [x] Texture cubemap à six faces carrées et clear/lecture indépendante de chaque face
- [x] Dimension au-delà de GL_MAX_TEXTURE_SIZE => Exception
- [x] Deux Surfaces WGL indépendantes, lecture de pixels distincts à handles identiques
- [ ] Test d'intégration dédié de reclaimReleased() pendant un tick sans consommateur (chemin implémenté)
- [x] Fermeture et callbacks exceptions suivis du clear ; tests d'ordre d'état, native destruction et idempotence
- [ ] Test instrumenté de l'ordre exact notification → clear GPU → destruction WGL
- [ ] Test d'injection des limites GL_MAX_COLOR_ATTACHMENTS/GL_MAX_DRAW_BUFFERS et des formats indisponibles.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Test GPU confirme depth unique dans opaque/WBOIT et absence de mutation des snapshots anciens.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

GLsync spécialisé, allocation GPU sur thread Update, deuxième gestionnaire de ressources.

## Consigne de passation

Confier **uniquement PR-11** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).

## État d'implémentation de la PR #60 (9 novembre 2026)

L'implémentation initiale a été poussée sur `impl/v0.1.4/11-gpu-attachments-surfaces`.
Elle n'introduit ni gestionnaire supplémentaire de ressources GPU, ni modification du rendu des widgets. La destination de rendu est désormais représentée par le petit type logique `Window::Surface::RenderTarget` (aucun nouveau framebuffer natif n'est alloué pour la fenêtre). Les classes `GPUResourceCollection`, `GPUResource::State`
et `GPUResource::Handle` sont conservées.

### API effective

- `Texture::createRenderTarget(Target, size, Format)` crée uniquement une description CPU.
- `Texture::Format` expose `RGBA16F` et `SRGB8_ALPHA8` en complément de `Depth32F` existant.
- `Framebuffer(size, vector<Texture::Handle>, optional<Texture::Handle>)` construit un framebuffer partageable.
- `CubeMap(faceSize, format)` expose une ressource six faces, profondeur ou couleur, et `CubeMap::Face` (`PositiveX`, `NegativeX`, etc.).
- `Framebuffer::Attachment{Texture::Handle, optional<CubeMap::Face>}` décrit une texture et, pour les cubemaps, la face **fixe** à attacher.
- `Framebuffer(size, vector<Framebuffer::Attachment>, optional<Framebuffer::Attachment>)` capture les bindings, avec validation des faces, types, tailles et formats.
- Chaque face peut être liée à son propre `Framebuffer::State`, tout en conservant le même `CubeMap::Handle` natif sur une Surface.
- Les constructeurs historiques utilisant les formats ou les `Texture::Handle` restent disponibles pour les textures 2D.
- `Framebuffer::colorAttachmentHandle(index)` et `depthStencilAttachmentHandle()` exposent l'identité exacte des handles.
- `Window::Surface::subscribeToDestruction(callback)` retourne un contrat RAII invalide après fermeture.

### Immutabilité et durée de vie

Le raccourci `Framebuffer(size, formats...)` garde son comportement de construction
mais `resize()` construit désormais un nouveau `Framebuffer::State` et de nouveaux
`Texture::State`; les anciens handles restent conservés par les snapshots.
Le constructeur à handles partagés interdit le resize vers une taille différente.

Une activation crée une instance native dans la `GPUResourceCollection` de la
`Window::Surface` cible ; il n'y a ni partage de pixels entre contextes WGL,
ni partage de framebuffer natif entre surfaces. Les faces cubemap sont choisies **lors de la construction des attachments**, immuablement dans le `Framebuffer::State`. L'activation n'utilise plus `Framebuffer::activateFace`, `TextureCubeFace` ni `RenderContext::framebufferCubeFace`. La sélection native intervient lors de la matérialisation GPU du framebuffer et n'est pas réajustée entre ses activations.

À la fermeture, les abonnés sont notifiés une seule fois après `Releasing` ;
la première exception de callback est retenue, les autres abonnés sont appelés,
puis les ressources GPU et WGL sont nettoyées, `Released` est atteint et cette
exception est relancée au seul appel explicite initial de `destroy()`.
Le runtime libère également l'enregistrement logique de la Surface en cas d'échec
d'un callback de destruction.

### Validation et réserves

Les TU CPU/OpenGL ajoutés vérifient les constructions partageables, les resizes,
les formats, les six faces cubemap (profondeur et couleur), la profondeur native partagée et l'isolation
des pixels WGL. Les tests de fermeture couvrent également plusieurs abonnés,
les exceptions, l'état terminal et les appels répétés.

**La PR ne peut pas être considérée terminée avant validation de la CI finale.**
Les résultats des jobs Debug/Release/Windows seront reportés dans la PR.
Les tests d'instrumentation de reclaim sans consommateur, de fermeture avant
clear GPU et d'injection explicite de limites GL restent à renforcer si la
CI actuelle n'apporte pas ces garanties.


### Orchestration du rendu dans un framebuffer

`RenderToFramebufferCommand(framebufferHandle, shared_ptr<const RenderSnapshot>)`
capture une version immuable du framebuffer et un sous-snapshot. À l'exécution,
la commande active la cible native propre à la Surface, ajuste le viewport à la
taille capturée, exécute le sous-snapshot et restaure les bindings OpenGL de
lecture/dessin, le viewport, le scissor et la précédente valeur de `RenderContext::activeRenderTarget`, y compris en cas d'exception imbriquée. Les
opérations de clear appartiennent aux commandes du sous-snapshot
(`ClearRenderCommand`) et ne sont pas imposées par la commande de cible.

Les tests de rendu des six faces couleur et profondeur utilisent effectivement
`RenderToFramebufferCommand`, avec un sous-snapshot comportant le clear.
Un test d'intégration supplémentaire invoque la commande depuis un
`RenderSnapshot::Builder` parent et vérifie la lecture des pixels ainsi que les
restaurations de contexte, l'erreur imbriquée et le resize versionné.

La classe `Window::Surface` déclare `RenderTarget` et expose
`defaultRenderTarget()`. Ce type emploie soit le framebuffer natif 0
(`Type::Default`, dimensions de `Surface::geometry()`), soit un
`Framebuffer::Handle` immuable (`Type::Framebuffer`, dimensions capturées).
Son implémentation se trouve dans le header graphique dédié
`core/window_surface_render_target.hpp`, de manière à ne pas introduire
de dépendance à GLEW dans `core/window.hpp`.

`RenderContext::activeRenderTarget` remplace `activeRenderTargetSize` :
`renderTargetSize()` délègue au `RenderTarget` actif. La
`RenderToFramebufferCommand` capture elle-même un `RenderTarget` et
restaure la cible précédente dans sa garde RAII, tout en préservant
séparément les anciens bindings natifs de lecture et dessin OpenGL.
Cela conserve les rendus imbriqués et les surfaces redimensionnables.

Le branchement automatique des sorties `EngineView` sur leurs propres
cibles et sur `EngineWidget` reste l'intégration de la PR12.

Les commandes `ViewportRenderCommand` et `ScissorRenderCommand` utilisent la taille de la cible active pour convertir leurs coordonnées d'origine supérieure gauche, ce qui permet de les employer dans des snapshots imbriqués hors écran sans modifier `Window`.
