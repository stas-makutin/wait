#ifndef _WIN32_WINNT
   #define _WIN32_WINNT    0x0502
#endif

#ifndef _WIN32_WINDOWS
   #define _WIN32_WINDOWS  0x0502
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <errno.h>
#include <string>
#include <vector>

// special return codes
#define RETURNCODE_SIGINT     (-1)
#define RETURNCODE_CLOSE      (-2)
#define RETURNCODE_LOGOFF     (-3)
#define RETURNCODE_SHUTDOWN   (-4)
#define RETURNCODE_HELP       (-5)
#define RETURNCODE_ERROR      (-6)

// command line parser states
#define ARGSTATE_NONE         (0)
#define ARGSTATE_DELTA        (1)
#define ARGSTATE_TIME         (2)
#define ARGSTATE_PROCESS      (3)

// time interval constant (unit is 100 nanoseconds)
#define ONE_MILLISECOND       (10000)
#define ONE_SECOND            (1000 * (ULONGLONG)ONE_MILLISECOND)
#define ONE_MINUTE            (60 * (ULONGLONG)ONE_SECOND)
#define ONE_HOUR              (60 * (ULONGLONG)ONE_MINUTE)
#define ONE_DAY               (24 * (ULONGLONG)ONE_HOUR)

// event type
enum eventType {
   EVENT_TIMEDELTA   = 0,
   EVENT_TIME,
   EVENT_PROCESS
};

// event data structure
typedef struct eventData {
   eventType      type;
   std::wstring   text;
   ULONGLONG      data;
   HANDLE         handle;
   
   eventData(eventType _type, const wchar_t* _text, ULONGLONG _data)
      : type(_type), text(_text), data(_data), handle(NULL)
   {
   }
} eventData;

// event list
typedef std::vector<eventData>   eventVector;
typedef std::vector<size_t>      indexVector;

// process info structure
typedef struct processInfo {
   DWORD          id;
   std::wstring   imageName;
   
   processInfo(DWORD _id) : id(_id)
   {
   }
} processInfo;

// process list
typedef std::vector<processInfo> processVector;

void print_title()
{
   puts(
"The wait utility. Allows to wait specific time or event(s).\r\n"
"Freeware by Stas Makutin (stas@makutin.net). Copyright 2007.\r\n"
   );
}

void print_help()
{
   print_title();
   puts(
"Usage: wait [-d <delta>] [-t <time>] [-p <process id> | <process name>]\r\n"
"            [-a] [-q]\r\n"
"\r\n"
"Options:\r\n"
" -h; -?; --help  : show this message.\r\n"
" -d; --delta     : time delta event. Wait specific time delta.\r\n"
" -t; --time      : time event. Wait till specific time.\r\n"
" -p; --process   : process event. Wait till end of specific process.\r\n"
"                   The process can be specified by its id or image name.\r\n"
" -a; --all       : wait all events. Without this option the program will exit\r\n" 
"                   when just one of events occurs.\r\n"
" -q; --quiet     : suppress any output, quiet mode.\r\n"
"\r\n"
"Formats:\r\n"
"1. Time delta format:\r\n"
"\r\n"
"[<number>D|d][<number>H|h][<number>M|m][<number>S|s][<number>]\r\n"
"\r\n"
"where:\r\n"
" number of D or d - number of days\r\n"
" number of H or h - number of hours\r\n"
" number of M or m - number of minutes\r\n"
" number of S or s - number of seconds\r\n"
" just number      - number of milliseconds\r\n"
"\r\n"
"2. Time format:\r\n"
"\r\n"
"a) <YYYY>-<MM>-<DD>T<HH>:<mm>:<SS>.<f>\r\n"
"b) <HH>:<mm>:<SS>.<f>\r\n"
"\r\n"
"where:\r\n"
" YYYY - year\r\n"
" MM   - month\r\n"
" DD   - day\r\n"
" HH   - hour\r\n"
" mm   - minute\r\n"
" SS   - second\r\n"
" f    - millisecond\r\n"
"\r\n"
"Return codes:\r\n"
"The program returns index of first occured event or one of special codes:\r\n"
"-1 - Ctrl+C or Ctrl+Break interruption\r\n"
"-2 - program was closed\r\n"
"-3 - user logoff event\r\n"
"-4 - system shutdown event\r\n"
"-5 - show help message\r\n"
"-6 - error occurs\r\n"
"negative number of events if you pass more than 32 events"
   );
}

void print_limit()
{
   print_title();
   puts(
"The maximum number of event cannot exceed 32."
   );
}

void print_handle_error()
{
   print_title();
   puts(
"The initialization error. Either not enough system resources or access rights."
   );
}

void print_event(const eventData* ed)
{
   std::wstring msg;

   switch (ed->type) {
   case EVENT_TIMEDELTA:   msg = L"Event: time delta "; break;
   case EVENT_TIME:        msg = L"Event: time "; break;
   case EVENT_PROCESS:     msg = L"Event: process "; break;
   }
   if (!msg.empty())
   {
      msg += ed->text;
      _putws( msg.c_str() );
   }
}

void print_special(int rc)
{
   if (RETURNCODE_CLOSE == rc)
   {
      puts("The program was closed.");
   }
   else if (RETURNCODE_LOGOFF == rc)
   {
      puts("The user was logged of.");
   }
   else if (RETURNCODE_SHUTDOWN == rc)
   {
      puts("The system is shutting down.");
   }
   else
   {
      puts("The wait was interrupted.");
   }
}

bool parse_delta(const wchar_t* str, ULONGLONG* value)
{
   bool rc = false;
   if (str && value)
   {
      unsigned long part;
      wchar_t* ptr1;
      wchar_t* ptr2 = const_cast<wchar_t*>(str);
      
      rc = true;
      *value = 0;
      
      while (rc && *ptr2)
      {
         ptr1 = ptr2;
         ptr2 = NULL;
         part = wcstoul(ptr1, &ptr2, 10);
         if (ERANGE == errno || ptr1 == ptr2)
         {
            rc = false;
         }
         else
         {
            if (!(*ptr2))
            {
               // milliseconds
               if (part > 0)
               {
                  *value += static_cast<ULONGLONG>(part) * ONE_MILLISECOND;
               }
            }
            else if (L'S' == *ptr2 || L's' == *ptr2)
            {
               // seconds
               if (part > 0)
               {
                  *value += static_cast<ULONGLONG>(part) * ONE_SECOND;
               }
               ptr2++;
            }
            else if (L'M' == *ptr2 || L'm' == *ptr2)
            {
               // minutes
               if (part > 0)
               {
                  *value += static_cast<ULONGLONG>(part) * ONE_MINUTE;
               }
               ptr2++;
            }
            else if (L'H' == *ptr2 || L'h' == *ptr2)
            {
               // hours
               if (part > 0)
               {
                  *value += static_cast<ULONGLONG>(part) * ONE_HOUR;
               }
               ptr2++;
            }
            else if (L'D' == *ptr2 || L'd' == *ptr2)
            {
               // days
               if (part > 0)
               {
                  *value += static_cast<ULONGLONG>(part) * ONE_DAY;
               }
               ptr2++;
            }
            else
            {
               rc = false;
            }
         }
      }
   }
   return rc;
}

bool parse_time(const wchar_t* str, ULONGLONG* value, const SYSTEMTIME* current)
{
   if (str && value)
   {
      int state = 0;
      unsigned long part;
      SYSTEMTIME stime;
      wchar_t* ptr1;
      wchar_t* ptr2 = const_cast<wchar_t*>(str);

      memcpy(&stime, current, sizeof(SYSTEMTIME));
      
      while ((state >= 0) && *ptr2)
      {
         ptr1 = ptr2;
         ptr2 = NULL;
         if (state > 0) ptr1++;
         part = wcstoul(ptr1, &ptr2, 10);
         if (ERANGE == errno || ptr1 == ptr2)
         {
            state = -2;
         }
         else
         {
            if (0 == state)
            {
               if (L'-' == *ptr2)
               {
                  // year
                  stime.wYear = static_cast<WORD>(part);
                  stime.wHour = stime.wMinute = stime.wSecond = stime.wMilliseconds = 0;
                  state = 1;
               }
               else if (L':' == *ptr2)
               {
                  // hour
                  stime.wHour = static_cast<WORD>(part);
                  stime.wMinute = stime.wSecond = stime.wMilliseconds = 0;
                  state = 4;
               }
               else
               {
                  state = -2;
               }
            }
            else if (1 == state)
            {
               if (!(*ptr2) || (L'-' == *ptr2))
               {
                  // month
                  stime.wMonth = static_cast<WORD>(part);
                  state = 2;
               }
               else
               {
                  state = -2;
               }
            }
            else if (2 == state)
            {
               if (!(*ptr2) || (L'T' == *ptr2) || (L't' == *ptr2))
               {
                  // day
                  stime.wDay = static_cast<WORD>(part);
                  state = 3;
               }
               else
               {
                  state = -2;
               }
            }
            else if (3 == state)
            {
               if (!(*ptr2) || (L':' == *ptr2))
               {
                  // hour
                  stime.wHour = static_cast<WORD>(part);
                  state = 4;
               }
               else
               {
                  state = -2;
               }
            }
            else if (4 == state)
            {
               if (!(*ptr2) || (L':' == *ptr2))
               {
                  // minute
                  stime.wMinute = static_cast<WORD>(part);
                  state = 5;
               }
               else
               {
                  state = -2;
               }
            }
            else if (5 == state)
            {
               if (!(*ptr2) || (L'.' == *ptr2) || (L':' == *ptr2))
               {
                  // second
                  stime.wSecond = static_cast<WORD>(part);
                  state = 6;
               }
               else
               {
                  state = -2;
               }
            }
            else if (6 == state)
            {
               stime.wMilliseconds = static_cast<WORD>(part);
               state = -1;
            }
         }
      }
      
      if (state > -2)
      {
         FILETIME ftime, ftimeUTC;
         
         if (::SystemTimeToFileTime( &stime, &ftime ))
         {
            if (::LocalFileTimeToFileTime( &ftime, &ftimeUTC ))
            {
               memcpy(value, &ftimeUTC, sizeof(ULONGLONG));
               return true;
            }
         }
      }
   }
   return false;
}

bool parse_process(const wchar_t* str, ULONGLONG* value)
{
   if (str && value)
   {
      const wchar_t* end = str + wcslen(str);
      
      while (str < end && iswspace(*str)) str++;
      while (str < end && iswspace(*(end - 1))) end--;

      if (str < end)
      {
         wchar_t* ptr = const_cast<wchar_t*>(end);
      
         unsigned long id = wcstoul(str, &ptr, 10);
         if (ERANGE == errno || ptr != end)
         {
            id = LONG_MAX;
         }
         *value = static_cast<ULONGLONG>(id);
         return true;
      }
   }
   return false;
}

void get_processes(processVector& processes)
{
   HANDLE hProcesses, hModules;
   PROCESSENTRY32 pe;
   MODULEENTRY32 me;
   
   hProcesses = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
   if (INVALID_HANDLE_VALUE != hProcesses)
   {
      pe.dwSize = sizeof(PROCESSENTRY32);
      me.dwSize = sizeof(MODULEENTRY32);
      if (Process32First( hProcesses, &pe ))
      {
         do
         {
            processInfo pi( pe.th32ProcessID );
         
            if (0 != pe.th32ProcessID)
            {
               hModules = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
               if (INVALID_HANDLE_VALUE != hModules)
               {
                  if (Module32First(hModules, &me))
                  {
                     pi.imageName.assign( me.szExePath );
                  }
                  ::CloseHandle( hModules );
               }
            }
            
            if (pi.imageName.empty())
            {
               pi.imageName.assign( pe.szExeFile );
            }
            
            processes.push_back( pi );
         }
         while (Process32Next( hProcesses, &pe ));
      }
      ::CloseHandle( hProcesses );
   }
}

bool is_in_string(const wchar_t* whole, size_t wholelen, const wchar_t* part, size_t partlen)
{
   if (whole == part)
   {
      return (partlen <= wholelen);
   }
   if (whole && part && partlen > 0 && partlen <= wholelen)
   {
      while (wholelen >= partlen)
      {
         if (CSTR_EQUAL == ::CompareStringW(
            LOCALE_INVARIANT, 
            NORM_IGNORECASE | SORT_STRINGSORT, 
            whole, static_cast<DWORD>(partlen), 
            part, static_cast<DWORD>(partlen))
         )
         {
            return true;
         }
         whole++;
         wholelen--;
      }
   }
   return false;
}

processInfo* find_process_info(const wchar_t* name, DWORD id, processVector& processes)
{
   if (LONG_MAX != id)
   {
      for (processVector::iterator it = processes.begin(); it != processes.end(); it++)
      {
         if (id == it->id)
         {
            return &(*it);
         }
      }
   }
   
   if (name)
   {
      const wchar_t* end = name + wcslen(name);
      while (name < end && iswspace(*name)) name++;
      while (name < end && iswspace(*(end - 1))) end--;   
      if (name < end)
      {
         std::wstring part( name, end - name );
         for (processVector::iterator it = processes.begin(); it != processes.end(); it++)
         {
            if (is_in_string(it->imageName.c_str(), it->imageName.size(), part.c_str(), part.size()))
            {
               return &(*it);
            }
         }
      }
   }
      
   return NULL;
}

HANDLE ctrlEvent = NULL;
int ctrlCode = RETURNCODE_SIGINT;

BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
   switch (dwCtrlType) {
   case CTRL_CLOSE_EVENT:
      ctrlCode = RETURNCODE_CLOSE;
      break;
   case CTRL_LOGOFF_EVENT:
      ctrlCode = RETURNCODE_LOGOFF;
      break;
   case CTRL_SHUTDOWN_EVENT:
      ctrlCode = RETURNCODE_SHUTDOWN;
      break;
   default:
      ctrlCode = RETURNCODE_SIGINT;
   }
   
   if (NULL != ctrlEvent)
   {
      ::SetEvent( ctrlEvent );
      return TRUE;
   }
   return FALSE;
}

int wmain(int argc, wchar_t *argv[])
{
   ULONGLONG   current_ftime;
   SYSTEMTIME  current_stime;
   bool        quiet       = false;
   bool        wait_all    = false;
   int         arg_state   = ARGSTATE_NONE;
   eventVector events;
   
   ::GetLocalTime( &current_stime );
   {
      FILETIME ft, ftu;
      if (!::SystemTimeToFileTime(&current_stime, &ft))
      {
         return RETURNCODE_ERROR;
      }
      if (!::LocalFileTimeToFileTime( &ft, &ftu ))
      {
         return RETURNCODE_ERROR;
      }
      memcpy(&current_ftime, &ftu, sizeof(ULONGLONG));
   }
   
   for (int argi = 0; argi < argc; argi++)
   {
      const wchar_t* arg = argv[ argi ];
      if (!arg) continue;      
   
      if (ARGSTATE_DELTA == arg_state)
      {
         ULONGLONG delta_value;
         if (parse_delta(arg, &delta_value))
         {
            events.push_back( eventData(EVENT_TIMEDELTA, arg, delta_value) );
         }
         arg_state = ARGSTATE_NONE;
      }
      else if (ARGSTATE_TIME == arg_state)
      {
         ULONGLONG time_value;
         if (parse_time(arg, &time_value, &current_stime))
         {
            events.push_back( eventData(EVENT_TIME, arg, time_value) );
         }
         arg_state = ARGSTATE_NONE;
      }
      else if (ARGSTATE_PROCESS == arg_state)
      {
         ULONGLONG process_value;
         if (parse_process(arg, &process_value))
         {
            events.push_back( eventData(EVENT_PROCESS, arg, process_value) );
         }
         arg_state = ARGSTATE_NONE;
      }
      else
      {
         bool long_options = false;
         if (L'-' == *arg)
         {
            arg++;
            if (L'-' == *arg)
            {
               long_options = true;
               arg++;
            }
         }
         else if (L'/' == *arg)
         {
            arg++;
         }
         else
         {
            arg = NULL;
         }
         
         if (arg && *arg)
         {
            if (long_options)
            {
               if (0 == _wcsicmp(arg, L"help"))
               {
                  events.clear();
                  break;
               }
               else if (0 == _wcsicmp(arg, L"delta"))
               {
                  arg_state = ARGSTATE_DELTA;
               }
               else if (0 == _wcsicmp(arg, L"time"))
               {
                  arg_state = ARGSTATE_TIME;
               }
               else if (0 == _wcsicmp(arg, L"process"))
               {
                  arg_state = ARGSTATE_PROCESS;
               }
               else if (0 == _wcsicmp(arg, L"all"))
               {
                  wait_all = true;
               }
               else if (0 == _wcsicmp(arg, L"quiet"))
               {
                  quiet = true;
               }
            }
            else
            {
               if (L'h' == *arg || L'H' == *arg || L'?' == *arg)
               {
                  events.clear();
                  break;
               }
               else if (L'd' == *arg || L'D' == *arg)
               {
                  arg_state = ARGSTATE_DELTA;
               }
               else if (L't' == *arg || L'T' == *arg)
               {
                  arg_state = ARGSTATE_TIME;
               }
               else if (L'p' == *arg || L'P' == *arg)
               {
                  arg_state = ARGSTATE_PROCESS;
               }
               else if (L'a' == *arg || L'A' == *arg)
               {
                  wait_all = true;
               }
               else if (L'q' == *arg || L'Q' == *arg)
               {
                  quiet = true;
               }
            }
         }
      }      
   }

   if (events.empty())
   {
      print_help();
      return RETURNCODE_HELP;
   }
   
   if (events.size() > 32)
   {
      if (!quiet)
      {
         print_limit();
      }
      return (-static_cast<int>(events.size()));
   }

   int rc = 0;

   // creating handles
   ctrlEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
   if (NULL == ctrlEvent)
   {
      rc = RETURNCODE_ERROR;
   }
   else   
   {
      processVector processes;
      for (eventVector::iterator it = events.begin(); it != events.end(); it++)
      {
         if (EVENT_PROCESS == it->type)
         {
            if (processes.empty())
            {
               get_processes( processes );
            }
            processInfo* pi = find_process_info( it->text.c_str(), static_cast<DWORD>(it->data), processes );
            if (NULL == pi)
            {
               if (!quiet)
               {
                  wprintf(L"Process %s not found\r\n", it->text.c_str());
               }
               it->text += L" (not found)";
            }
            else
            {
               if (!quiet)
               {
                  wprintf(L"Process %s found as: %s (%u)\r\n", it->text.c_str(), pi->imageName.c_str(), pi->id);
               }
               it->text += L" (";
               it->text += pi->imageName;
               it->text += L")";
               
               it->handle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi->id);
            }
            if (NULL == it->handle)
            {
               it->handle = ::CreateEvent(NULL, TRUE, TRUE, NULL);
               if (NULL == it->handle)
               {
                  rc = RETURNCODE_ERROR;
                  break;
               }
            }
         }
         else
         {
            it->handle = ::CreateWaitableTimer(NULL, TRUE, NULL);
            if (NULL == it->handle)
            {
               rc = RETURNCODE_ERROR;
               break;
            }
            
            LARGE_INTEGER time;
            if (EVENT_TIME == it->type)
            {
               time.QuadPart = it->data;
            }
            else
            {
               time.QuadPart = current_ftime + it->data;
            }
            
            if (!::SetWaitableTimer(it->handle, &time, 0, NULL, NULL, TRUE))
            {
               rc = RETURNCODE_ERROR;
               break;
            }
         }
      }
   }
   
   // execution
   if (0 == rc)
   {
      HANDLE* handles = new HANDLE[events.size() + 1];
      if (NULL == handles)
      {
         rc = RETURNCODE_ERROR;
      }
      else
      {
         indexVector indexes;
         size_t index, count = events.size();
         DWORD code;
         
         for (index = 0; index < count; index++)
         {
            handles[index] = events[index].handle;
            indexes.push_back(index);
         }
         handles[count] = ctrlEvent;
      
         ::SetConsoleCtrlHandler(ctrl_handler, TRUE);
         
         while (count > 0)
         {
            code = ::WaitForMultipleObjects(static_cast<DWORD>(count) + 1, handles, FALSE, INFINITE);
            if (code >= WAIT_OBJECT_0)
            {
               index = code - WAIT_OBJECT_0;
               if (index >= count)
               {
                  rc = ctrlCode;
                  if (!quiet) print_special(rc);
                  break;
               }

               if (!quiet) print_event( &events[ indexes[index] ] );
               
               if (!wait_all)
               {
                  rc = static_cast<int>( index );
                  break;
               }

               memmove(&handles[index], &handles[index + 1], (count - index) * sizeof(HANDLE));
               indexes.erase( indexes.begin() + index );
               count--;
            }
         }
         
         delete[] handles;
      }
   }

   // clean up handles
   for (eventVector::iterator it = events.begin(); it != events.end(); it++)
   {
      ::CloseHandle( it->handle );
      it->handle = NULL;
   }
   
   if (NULL != ctrlEvent)
   {
      ::CloseHandle(ctrlEvent);
      ctrlEvent = NULL;
   }
   
   if (RETURNCODE_ERROR == rc && (!quiet))
   {
      print_handle_error();
   }

	return rc;
}

