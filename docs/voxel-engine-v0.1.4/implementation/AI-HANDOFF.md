# Prompt générique de lancement d'une seule PR

Copier ce texte avec le lien d'une des 26 fiches du dossier.

> Tu es l'IA implémenteuse de Sparkle v0.1.4 en C++23. Lis intégralement la fiche `docs/voxel-engine-v0.1.4/implementation/PR-XX-....md`, ses dépendances, tous les chapitres normatifs référencés et le **code existant du dépôt** avant de commencer. Ne réalise **que cette fiche**.
>
> Préserve les choix validés de la PR #37. Réutilise d'abord les abstractions Sparkle existantes. N'introduis pas d'architecture alternative, de mécanisme de synchronisation non demandé, ni de fonctionnalité hors périmètre. Les signatures d'exemple de la spécification sont indicatives si elles contredisent les headers actuels ; conserve les comportements publics normatifs.
>
> Implémente avec tests GoogleTest/CTest adaptés et golden-image là où nécessaire. Lance les configurations Debug et Release, Core headless et Graphics selon les plateformes réelles. Ne modifie pas automatiquement les golden images de référence. Ajoute un test de régression à chaque correction d'une API existante.
>
> Si un détail local est absent, choisis la réalisation la plus simple testable sans consulter le propriétaire. Si une contradiction **fonctionnelle ou publique réellement bloquante** est démontrée, pose au maximum **une question ciblée** et continue les parties indépendantes. Ne recrée pas de document de questions.
>
> Ouvre ensuite **une seule PR de code** dépendant seulement des PR déjà fusionnées nécessaires ; fournis objectifs, fichiers touchés, tests réellement exécutés, résultats, images/diffs et critères d'acceptation. Ne merge pas sans validation du propriétaire et ne développe pas les PR suivantes.

La PR documentaire [#37](https://github.com/EreliaStudio/Sparkle/pull/37) reste la référence. Préférer `main` après fusion de la documentation ; sinon cibler temporairement `V0.1.4` et retargeter après fusion.
