/* utf8test.rexx - utf8.library's ARexx functions, each checked.
   rx utf8test.rexx   (RexxMast must be running) */
fails = 0

IF ~SHOW('L', 'utf8.library') THEN
  IF ~ADDLIB('utf8.library', 0, -30, 0) THEN DO
    SAY 'utf8test.rexx: utf8.library would not add'
    EXIT 20
  END

gbg = 'G' || 'C3B6'x || 'teborg'            /* Göteborg in UTF-8 */
CALL check UTF8LEN(gbg) = 8, 'UTF8LEN counts characters'
CALL check LENGTH(gbg) = 9, '(ARexx LENGTH counts bytes)'
CALL check UTF8LEN('a' || 'CC8A'x) = 1, 'a + ring is one character'
CALL check UTF8WIDTH('E697A5E69CAC'x) = 4, 'UTF8WIDTH: two wide characters'
CALL check UTF8LEFT('E697A5E69CAC'x, 3) = 'E697A5'x, 'UTF8LEFT: no half character'
CALL check UTF8SUBSTR(gbg, 2, 3) = 'C3B6'x || 'te', 'UTF8SUBSTR'
CALL check UTF8SUBSTR(gbg, 5) = 'borg', 'UTF8SUBSTR to the end'
CALL check UTF8SUBSTR(gbg, 20) = '', 'UTF8SUBSTR past the end'
CALL check UTF8UPPER(gbg) = 'G' || 'C396'x || 'TEBORG', 'UTF8UPPER'
CALL check UTF8LOWER('D096'x) = 'D0B6'x, 'UTF8LOWER Cyrillic'
CALL check UTF8TO(gbg) = 'G' || 'F6'x || 'teborg', 'UTF8TO Latin-1'
CALL check UTF8TO('E282AC'x, 'latin9') = 'A4'x, 'UTF8TO Latin-9 euro'
CALL check UTF8TO('E282AC'x) = 'EUR', 'UTF8TO Latin-1 euro: EUR'
CALL check UTF8FROM('G' || 'F6'x || 'teborg') = gbg, 'UTF8FROM Latin-1'
CALL check UTF8FROM('DAC4'x, 'CP437') = 'E2948CE29480'x, 'UTF8FROM CP437 box'
CALL check UTF8NORMALIZE('a' || 'CC8A'x) = 'C3A5'x, 'UTF8NORMALIZE together'
CALL check UTF8NORMALIZE('C3A5'x, 'NFD') = 'a' || 'CC8A'x, 'UTF8NORMALIZE apart'
CALL check UTF8CHECK('plain') = 'ASCII', 'UTF8CHECK ASCII'
CALL check UTF8CHECK(gbg) = 'UTF8', 'UTF8CHECK UTF8'
CALL check UTF8CHECK('F6'x) = 'LATIN1', 'UTF8CHECK LATIN1'
CALL check UTF8CHAR(229) = 'C3A5'x, 'UTF8CHAR decimal'
CALL check UTF8CHAR('U+1F600') = 'F09F9880'x, 'UTF8CHAR U+'
CALL check UTF8CHAR(72, 105) = 'Hi', 'UTF8CHAR several'
CALL check UTF8CODE('C3A5'x) = 229, 'UTF8CODE'
CALL check UTF8CODE('aC3A5'x, 2) = 229, 'UTF8CODE the second'
CALL check UTF8FIND(gbg, 'borg') = 5, 'UTF8FIND in characters'
CALL check UTF8FIND(gbg, 'GOTE', 'NOCASE NOACCENT') = 1, 'UTF8FIND NOCASE NOACCENT'
CALL check UTF8FIND(gbg, 'x') = 0, 'UTF8FIND not there'
CALL check UTF8COMPARE('C3A5'x, 'a' || 'CC8A'x) = 0, 'UTF8COMPARE: å either way'
CALL check UTF8COMPARE('abc', 'ABD', 'NOCASE') = -1, 'UTF8COMPARE NOCASE'

IF fails = 0 THEN SAY 'utf8test.rexx: all passed'
ELSE SAY 'utf8test.rexx:' fails 'failed'
EXIT fails > 0

check: PROCEDURE EXPOSE fails
  PARSE ARG ok, what
  IF ~ok THEN DO
    SAY 'failed:' what
    fails = fails + 1
  END
  RETURN
