RESULT=0
SLEEP_CHECK="null"
SLEEP_CHECK=`nc -l -u -p 8002 -w 0`
if echo "$SLEEP_CHECK" | grep -q "wake" ;
then
	RESULT=1
fi
if echo "$SLEEP_CHECK" | grep -q "sleep" ;
then
	RESULT=2
fi
exit "$RESULT"
