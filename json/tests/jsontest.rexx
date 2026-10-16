/* jsontest.rexx - json.library's ARexx functions, each checked.
   rx jsontest.rexx   (RexxMast must be running) */
fails = 0

IF ~SHOW('L', 'json.library') THEN
  IF ~ADDLIB('json.library', 0, -30, 0) THEN DO
    SAY 'jsontest.rexx: json.library would not add'
    EXIT 20
  END

/* reading */
j = JSONPARSE('{"weather":{"city":"G' || 'C3B6'x || 'teborg","temp":8.25,"days":[1,2,3],"ok":true,"x":null}}')
CALL check j ~= '', 'JSONPARSE gives a handle'
CALL check JSONGET(j, 'weather.city') == 'G' || 'F6'x || 'teborg', 'a string as Latin-1'
CALL check JSONGET(j, 'weather.temp') == '8.25', 'a number as its text'
CALL check JSONGET(j, 'weather.days[2]') == '3', 'an array item'
CALL check JSONGET(j, 'weather.days[-1]') == '3', 'the last item'
CALL check JSONGET(j, 'weather.ok') == '1', 'true as 1'
CALL check JSONGET(j, 'weather.x') == '', 'null as nothing'
CALL check JSONGET(j, 'weather.none', 'no') == 'no', 'a default when missing'
CALL check JSONGET(j, 'weather.days') == '[1,2,3]', 'an array as JSON'
CALL check JSONTYPE(j, 'weather') == 'OBJECT', 'JSONTYPE object'
CALL check JSONTYPE(j, 'weather.temp') == 'NUMBER', 'JSONTYPE number'
CALL check JSONTYPE(j, 'weather.none') == '', 'JSONTYPE missing'
CALL check JSONCOUNT(j, 'weather') == 5, 'JSONCOUNT'
CALL check JSONNAME(j, 'weather', 1) == 'temp', 'JSONNAME'
CALL check JSONNAME(j, 'weather', 9) == '', 'JSONNAME past the end'
CALL check JSONFREE(j) == 1, 'JSONFREE'

/* errors */
CALL check JSONPARSE('{"a" 1}') == '', 'bad JSON gives no handle'
CALL check JSONERROR() == '1:6 expected : after a name', 'JSONERROR: ' || JSONERROR()
CALL check JSONPARSE('// c' || '0a'x || '[1,]', 'RELAXED') ~= '', 'RELAXED'

/* building */
k = JSONNEW()
CALL check JSONSET(k, 'city', 'G' || 'F6'x || 'teborg') == 1, 'JSONSET text'
CALL check JSONSET(k, 'temp', '8.5') == 1, 'JSONSET number'
CALL check JSONSET(k, 'zip', '01234') == 1, 'JSONSET leading zero stays text'
CALL check JSONSET(k, 'code', '42', 'STRING') == 1, 'JSONSET forced text'
CALL check JSONSET(k, 'sunny', 'yes', 'BOOL') == 1, 'JSONSET bool'
CALL check JSONSET(k, 'rain', '', 'NULL') == 1, 'JSONSET null'
CALL check JSONSET(k, 'days[0].t', 1) == 1, 'JSONSET makes an array and an object'
CALL check JSONSET(k, 'days[1].t', 2) == 1, 'JSONSET adds to the array'
CALL check JSONSET(k, 'raw', '{"a":[true]}', 'JSON') == 1, 'JSONSET JSON'
CALL check JSONSET(k, 'days[5]', 1) == 0, 'JSONSET no holes'
CALL check JSONERROR() == 'the path cannot be set there', 'and why: ' || JSONERROR()
CALL check JSONSET(k, 'n', 'abc', 'NUMBER') == 0, 'JSONSET not a number'
CALL check JSONSET(k, 'bad', '{', 'JSON') == 0, 'JSONSET bad JSON'
CALL check JSONERROR() == '1:2 the text ends too soon', 'and why: ' || JSONERROR()
want = '{"city":"G\u00f6teborg","temp":8.5,"zip":"01234","code":"42","sunny":true,"rain":null,' ||,
       '"days":[{"t":1},{"t":2}],"raw":{"a":[true]}}'
CALL check JSONTEXT(k, '', 'ASCII') == want, 'JSONTEXT: ' || JSONTEXT(k, '', 'ASCII')
CALL check JSONSET(k, 'temp', '9') == 1, 'JSONSET replaces'
CALL check JSONGET(k, 'temp') == '9', 'the new value'
CALL check JSONREMOVE(k, 'raw') == 1, 'JSONREMOVE'
CALL check JSONTYPE(k, 'raw') == '', 'removed'
CALL check JSONSAVE(k, 'T:jsontest.json', 'PRETTY') == 1, 'JSONSAVE'
l = JSONLOAD('T:jsontest.json')
CALL check JSONGET(l, 'days[1].t') == '2', 'JSONLOAD reads it back'
CALL check JSONGET(l, 'city') == 'G' || 'F6'x || 'teborg', 'and the Latin-1 survived'
CALL JSONFREE(l)
CALL JSONFREE(k)
ADDRESS COMMAND 'Delete T:jsontest.json QUIET'

IF fails = 0 THEN SAY 'jsontest.rexx: all passed'
ELSE SAY 'jsontest.rexx:' fails 'failed'
EXIT fails ~= 0

check: PROCEDURE EXPOSE fails
  PARSE ARG ok, what
  IF ~ok THEN DO
    SAY 'FAILED:' what
    fails = fails + 1
  END
  RETURN
