# PR04 — ChunkRequester asynchrone et source locale
**Couche :** Core. **Dépendances :** PR03. **État :** cahier des charges.

## Contrat
- ChunkRequester<TChunk> : interface polymorphe request(ChunkCoordinate) qui rend une réponse spk::Task<std::optional<TChunk>>::Answer ; une Task par demande. Les sources futures peuvent lire disque ou réseau sans nouveau protocole requis.
- Source locale : délègue Generator::generate(coord) à spk::WorkerPool. Le résultat est un std::optional<TChunk> : valeur => chunk, nullopt => absence valide ; Task::Failed => erreur, distincte de nullopt. WorkerPool capture les exceptions et remplit Task::Failed.
- Aucune allocation owning-pointer obligatoire dans l'API utilisateur pour le résultat. L'actuel Task::Answer::result() ne renvoie qu'une **référence const** ; ainsi le transfert dans ChunkCollection nécessite un TChunk copiable ou une modification distincte et expressément approuvée de Task. Pour v0.1.5, le contrat des chunks prévoit la copie de valeur, sans recopier les abonnements.
- Aucune mutation d'Engine, de ChunkCollection ou d'Entity sur un worker ; le streamer publiera sur le thread Engine.
- Assurer une durée de vie sûre des opérations : les closures worker ne doivent pas dereférencer Generator/Requester détruit. Exiger que les dépendances externes soient maintenues en vie jusqu'à achèvement, ou capturer un état indépendant explicitement propriétaire selon l'implémentation ; aucun pointeur brut vers le streamer dans un callback.
- La destruction du streamer est non bloquante : il se désabonne et ignore les résultats. **La destruction de WorkerPool elle-même peut attendre ses jthread** ; ne jamais prétendre qu'elle annule ou termine instantanément une génération infinie. Le Pool et les ressources de génération doivent survivre tant que les workers en ont besoin. À la destruction de WorkerPool, des jobs encore en file peuvent rester sans exécution : leurs Answer pourraient rester Pending. Il ne faut donc pas détruire le Pool avant d'avoir arbitré ses demandes soumises, ni promettre leur terminaison après la destruction du Pool.

## Tests
Trois issues (valeur/nullopt/Failed), exceptions propagées comme erreurs, tâches terminées avant l'enregistrement du callback, source mock distante, shutdown streamer avec tâche encore en cours, accès non bloquant aux réponses prêtes.

## Exclusions
Pas de politique de timeout/retry/ordonnancement (PR06), pas d'annulation de Task ni de refonte du WorkerPool.
