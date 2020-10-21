Root folder:

avec2012/

place scripts and configs into 
avec2012/featureextraction

extract the train/test/devel zips into
avec2012/audio

so that you have
avec2012/audio/devel/wav/
avec2012/audio/devel/words/
...

then 
cd avec2012/featureextraction

and run "perl extr.pl"

This creates the features in 
avec2012/arff/

For word-level audio features,
(voiced) segment level features,
and phrase level features

and the LLDs based on voiced segments

All the feature files include frame timestamps and durations (except for the LLD files). These are the two fields following the instance ID string (they are relative to the start time found in the instance ID strin). The instance ID string contains the start time (seconds) of the current word, phrase, or voiced segment relative to the start of the audio file. 

Voiced segment level functionals follow an incremental and overlapping sampling :   every 0.5s a 2s window is sampled 
Phrase and word functionals are computed over the whole unit (phrase or word)

Phrase and voiced segment boundaries are put when a pause between words is > 1.0 seconds.
The threshold is decreased linearly for phrase boundaries it make a phrase end more likely and force it at the next pause > 0 after 20 seconds. (i.e. after 10 seconds phrase duration the threshold will be 0.5 seconds)

Currently missing: averaging of 4 LLD frames (~resampling of LLD to match the video rate...)
