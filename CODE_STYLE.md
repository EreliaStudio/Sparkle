Const ref vs copy parameter
---> I would like to move to the new code style of C++11, so move to copy parameter, for constructor mainly, or for any function that take ownership of a data.
Note that this mean that we need to duplicate the data passed before passing it to destination, which mean reverse order of what i think of -> need to re-learn this order as it can change things


using value_type = TType;
using generator = std::function<TType()>;
using destructor = std::function<void(TType &)>;
---> A bit uneasy about this kind of using, as the syntax norm i want to use is pascal case. Its still in snake case due to standard library using "value_type" etc, i wonder if i should move them to sparkle definition, as it would feel more logical with the rest of the library