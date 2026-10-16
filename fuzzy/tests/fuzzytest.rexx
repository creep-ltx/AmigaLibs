/* fuzzytest.rexx - fuzzy.library's ARexx functions, each checked.
   rx fuzzytest.rexx   (RexxMast running; FUZZYLOCATE uses the index the
   ENV: setting FuzzyFind/Index names, made with fuzzytest or updatedb) */
fails = 0
IF ~SHOW('L', 'fuzzy.library') THEN
  IF ~ADDLIB('fuzzy.library', 0, -30, 0) THEN DO
    SAY 'fuzzytest.rexx: fuzzy.library would not add'
    EXIT 20
  END

CALL check FUZZYSCORE('csrc', 'config/source.c') > 0, 'a match'
CALL check FUZZYSCORE('csrc', 'config/sauce.c') = 0, 'no match'
CALL check FUZZYSCORE('goteborg', 'G' || 'F6'x || 'teborg') > 0, 'accents folded'
CALL check FUZZYSCORE('goteborg', 'G' || 'F6'x || 'teborg', 'ACCENTS') = 0, 'ACCENTS'
CALL check FUZZYSCORE('WORK', 'Work:x', 'case') = 0, 'CASE'
CALL check FUZZYSCORE('src', 'Work:s/r/c', 'PLAIN') = 0, 'PLAIN'
CALL check FUZZYSCORE('dpaint', 'Work:Graphics/DPaint/DPaint') >,
           FUZZYSCORE('dpaint', 'Work:Docs/dpaint-notes/a.txt'), 'a name beats a drawer'

hits = FUZZYLOCATE('dpaint', 3)
CALL check hits ~= '', 'FUZZYLOCATE found something: ' || FUZZYERROR()
SAY 'FUZZYLOCATE dpaint, 3:'
SAY hits
CALL check FUZZYLOCATE('qqqqzzzz') = '', 'nothing found is empty'

IF fails = 0 THEN SAY 'fuzzytest.rexx: all passed'
ELSE SAY 'fuzzytest.rexx:' fails 'failed'
EXIT fails ~= 0

check: PROCEDURE EXPOSE fails
  PARSE ARG ok, what
  IF ~ok THEN DO
    SAY 'FAILED:' what
    fails = fails + 1
  END
  RETURN
