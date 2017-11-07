The wait utility (for Windows).  
Allows to wait specific time or event(s).  
Freeware by Stas Makutin (stas@makutin.net). Copyright 2007.  
  
Usage: wait [-d \<delta\>] [-t \<time\>] [-p \<process id\> | \<process name\>] [-a] [-q]  
  
Options:  
&nbsp;&nbsp;-h; -?; --help  : show this message.  
&nbsp;&nbsp;-d; --delta     : time delta event. Wait specific time delta.  
&nbsp;&nbsp;-t; --time      : time event. Wait till specific time.  
&nbsp;&nbsp;-p; --process   : process event. Wait till end of specific process. The process can be specified by its id or image name.  
&nbsp;&nbsp;-a; --all       : wait all events. Without this option the program will exit when just one of events occurs.  
&nbsp;&nbsp;-q; --quiet     : suppress any output, quiet mode.  
  
Formats:  
1. Time delta format:  
  
[\<number\>D|d][\<number\>H|h][\<number\>M|m][\<number\>S|s][\<number\>]
  
where:  
&nbsp;&nbsp;number of D or d - number of days  
&nbsp;&nbsp;number of H or h - number of hours  
&nbsp;&nbsp;number of M or m - number of minutes  
&nbsp;&nbsp;number of S or s - number of seconds  
&nbsp;&nbsp;just number      - number of milliseconds  
  
2. Time format:  
  
a) \<YYYY\>-\<MM\>-\<DD\>T\<HH\>:\<mm\>:\<SS\>.\<f\>  
b) \<HH\>:\<mm\>:\<SS\>.\<f\>  
  
where:  
&nbsp;&nbsp;YYYY - year  
&nbsp;&nbsp;MM   - month  
&nbsp;&nbsp;DD   - day  
&nbsp;&nbsp;HH   - hour  
&nbsp;&nbsp;mm   - minute  
&nbsp;&nbsp;SS   - second  
&nbsp;&nbsp;f    - millisecond  
  
Return codes:  
The program returns index of first occured event or one of special codes:  
-1 - Ctrl+C or Ctrl+Break interruption  
-2 - program was closed  
-3 - user logoff event  
-4 - system shutdown event  
-5 - show help message  
-6 - error occurs  
negative number of events if you pass more than 32 events  
