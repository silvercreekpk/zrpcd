/* zrpc debug routines
 * Copyright (c) 2016 6WIND,
 *
 * This file is part of ZRPC daemon.
 *
 * See the LICENSE file.
 */
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <syslog.h>

#include "vty.h"
#include "command.h"

#include "zrpcd/zrpc_memory.h"
#include "zrpcd/zrpc_debug.h"
#include "zrpcd/zrpc_thrift_wrapper.h"
#include "zrpcd/bgp_configurator.h"
#include "zrpcd/bgp_updater.h"
#include "zrpcd/zrpc_bgp_configurator.h"
#include "zrpcd/zrpc_bgp_updater.h"
#include "zrpcd/zrpc_vpnservice.h"

#define ZRPC_STR "ZRPC Information\n"

typedef enum 
{
  ZRPC_LOG_LEVEL_EMERGENCIES = 0,
  ZRPC_LOG_LEVEL_ALERTS,
  ZRPC_LOG_LEVEL_CRITICAL,
  ZRPC_LOG_LEVEL_ERRORS,
  ZRPC_LOG_LEVEL_WARNINGS,
  ZRPC_LOG_LEVEL_NOTIFICATIONS,
  ZRPC_LOG_LEVEL_INFORMATIONAL,
  ZRPC_LOG_LEVEL_DEBUG
} zrpc_log_level_t;

const char *zrpc_log_level_str[]={"Emergency",
                            "Alert",
                            "Critical",
                            "Error",
                            "Warning",
                            "Notice",
                            "Informational",
                            "Debug"};

unsigned int log_stdout = 1;
unsigned int log_syslog = 1;
unsigned int record_priority = 1;
unsigned int log_facility = LOG_DAEMON;
FILE *log_file_fp = NULL;
char *log_file_filename = NULL;
zrpc_log_level_t log_level = ZRPC_LOG_LEVEL_DEBUG;
/* For debug statement. */
unsigned long zrpc_debug = 0;

static zrpc_log_level_t
zrpc_level_match(const char *s);


static zrpc_log_level_t
zrpc_level_match(const char *s)
{
  int level ;
  
  for ( level = 0 ; zrpc_log_level_str [level] != NULL ; level ++ )
    if (!strncmp (s, zrpc_log_level_str[level], 3 ))
      return level;
  return ZRPC_LOG_LEVEL_DEBUG;
}

DEFUN (show_debugging_zrpc_stats,
       show_debugging_zrpc_stats_cmd,
       "show debugging zrpc stats",
       SHOW_STR
       DEBUG_STR
       ZRPC_STR
       "Zrpc Statistics")
{
  struct zrpc_vpnservice *ctxt = NULL;

  zrpc_vpnservice_get_context (&ctxt);
  if(!ctxt)
    {
      return CMD_SUCCESS;
    }
  vty_out (vty, "BGP ZMQ notifications total %u lost %u thrift lost %u%s",
           ctxt->bgp_update_total,
           ctxt->bgp_update_lost_msgs,
           ctxt->bgp_update_thrift_lost_msgs,
           VTY_NEWLINE);
  return CMD_SUCCESS;
}

DEFUN (show_debugging_zrpc_errno,
       show_debugging_zrpc_errno_cmd,
       "show debugging zrpc errno",
       SHOW_STR
       DEBUG_STR
       ZRPC_STR
       "Zrpc errno with 6644")
{
  int i;
  for (i = 0 ; i < ZRPC_MAX_ERRNO; i++)
    {
      if (notification_socket_errno[i] > 0)
        {
          vty_out (vty, " %s(%u) = %u%s", strerror(i), i, notification_socket_errno[i], VTY_NEWLINE);
        }
    }
  return CMD_SUCCESS;
}

DEFUN (show_debugging_zrpc,
       show_debugging_zrpc_cmd,
       "show debugging zrpc",
       SHOW_STR
       DEBUG_STR
       ZRPC_STR)
{
  vty_out (vty, "ZRPC debugging status:%s", VTY_NEWLINE);

  if (IS_ZRPC_DEBUG)
    vty_out (vty, "  ZRPC debugging is on%s", VTY_NEWLINE);
  if (IS_ZRPC_DEBUG_NETWORK)
    vty_out (vty, "  ZRPC debugging network is on%s", VTY_NEWLINE);
  if (IS_ZRPC_DEBUG_NOTIFICATION)
    vty_out (vty, "  ZRPC debugging notification is on%s", VTY_NEWLINE);
  if (IS_ZRPC_DEBUG_CACHE)
    vty_out (vty, "  ZRPC debugging cache is on%s", VTY_NEWLINE);
  return CMD_SUCCESS;
}

DEFUN (debug_zrpc,
       debug_zrpc_cmd,
       "debug zrpc",
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug |= ZRPC_DEBUG;
  return CMD_WARNING;
}

DEFUN (no_debug_zrpc,
       no_debug_zrpc_cmd,
       "no debug zrpc",
       NO_STR
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug &= ~ZRPC_DEBUG;
  return CMD_SUCCESS;
}

DEFUN (debug_zrpc_notification,
       debug_zrpc_notification_cmd,
       "debug zrpc notification",
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug |= ZRPC_DEBUG_NOTIFICATION;
  return CMD_WARNING;
}

DEFUN (no_debug_zrpc_notification,
       no_debug_zrpc_notification_cmd,
       "no debug zrpc notification",
       NO_STR
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug &= ~ZRPC_DEBUG_NOTIFICATION;
  return CMD_SUCCESS;
}

DEFUN (debug_zrpc_network,
       debug_zrpc_network_cmd,
       "debug zrpc network",
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug |= ZRPC_DEBUG_NETWORK;
  return CMD_WARNING;
}

DEFUN (no_debug_zrpc_network,
       no_debug_zrpc_network_cmd,
       "no debug zrpc network",
       NO_STR
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug &= ~ZRPC_DEBUG_NETWORK;
  return CMD_SUCCESS;
}

DEFUN (debug_zrpc_cache,
       debug_zrpc_cache_cmd,
       "debug zrpc cache",
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug |= ZRPC_DEBUG_CACHE;
  return CMD_WARNING;
}

DEFUN (no_debug_zrpc_cache,
       no_debug_zrpc_cache_cmd,
       "no debug zrpc cache",
       NO_STR
       DEBUG_STR
       ZRPC_STR
       "ZRPC\n")
{
  zrpc_debug &= ~ZRPC_DEBUG_CACHE;
  return CMD_SUCCESS;
}

/* Debug node. */
static struct cmd_node debug_node =
{
  DEBUG_NODE,
  "",				/* Debug node has no interface. */
  1
};

static int
config_write_debug (struct vty *vty)
{
  int write = 0;

  if (IS_ZRPC_DEBUG)
    {
      vty_out (vty, "debug zrpc%s", VTY_NEWLINE);
      write++;
    }
  if (IS_ZRPC_DEBUG_NOTIFICATION)
    {
      vty_out (vty, "debug zrpc notification%s", VTY_NEWLINE);
      write++;
    }
  if (IS_ZRPC_DEBUG_NETWORK)
    {
      vty_out (vty, "debug zrpc network%s", VTY_NEWLINE);
      write++;
    }
  if (IS_ZRPC_DEBUG_CACHE)
    {
      vty_out (vty, "debug zrpc cache%s", VTY_NEWLINE);
      write++;
    }
  return write;
}

void
zrpc_debug_reset (void)
{
  zrpc_debug = 0;
}

void
zrpc_debug_init (void)
{
  zrpc_debug = 0;

  install_node (&debug_node, config_write_debug);
  install_element (ENABLE_NODE, &show_debugging_zrpc_cmd);
  install_element (ENABLE_NODE, &debug_zrpc_cmd);
  install_element (ENABLE_NODE, &no_debug_zrpc_cmd);
  install_element (ENABLE_NODE, &debug_zrpc_notification_cmd);
  install_element (ENABLE_NODE, &no_debug_zrpc_notification_cmd);
  install_element (ENABLE_NODE, &debug_zrpc_network_cmd);
  install_element (ENABLE_NODE, &no_debug_zrpc_network_cmd);
  install_element (ENABLE_NODE, &debug_zrpc_cache_cmd);
  install_element (ENABLE_NODE, &no_debug_zrpc_cache_cmd);
  install_element (ENABLE_NODE, &show_debugging_zrpc_stats_cmd);
  install_element (ENABLE_NODE, &show_debugging_zrpc_errno_cmd);

  zrpc_debug |= ZRPC_DEBUG;
  zrpc_debug |= ZRPC_DEBUG_NOTIFICATION;

  openlog("zrpcd", LOG_CONS|LOG_NDELAY|LOG_PID, log_facility);
}

char dest_sys[1024];

static inline void zrpc_system ( char *text)
{
  FILE *fp = fopen(log_file_filename, "a+");
  if (fp == NULL){
    return;
  }
  fprintf (fp, "%s\r\n", text);
  fflush (fp);
  fclose (fp);
}

void
zrpc_log(const char *format, ...)
{
  time_t t;
  char buffer[50];
  struct tm* tm_info;
  static char dest[1024];
  va_list argptr;

  if (log_level < ZRPC_LOG_LEVEL_DEBUG)
    return;

  time (&t);
  tm_info = localtime(&t);
  strftime(buffer, 26, "%Y/%m/%d %H:%M:%S", tm_info);

  va_start(argptr, format);
  vsprintf(dest, format, argptr);
  va_end(argptr);

  if (log_stdout)
    fprintf(stderr, "%s %s ZRPC: %s\r\n",
            buffer, record_priority ?
            zrpc_log_level_str[ZRPC_LOG_LEVEL_DEBUG] : "", dest);
  if (log_syslog)
    syslog(ZRPC_LOG_LEVEL_DEBUG | log_facility, "%s", dest);
  if (!log_file_filename)
    return;
  //log_file_fp = fopen (log_file_filename, "a");
  //if (log_file_fp == NULL)
  //  return ;
  sprintf(dest_sys, "%s %s ZRPC: %s",
            buffer, record_priority ?
            zrpc_log_level_str[ZRPC_LOG_LEVEL_DEBUG] : "", dest);
  zrpc_system (dest_sys);
  //fprintf (log_file_fp, "%s ZRPC: %s\r\n", buffer, dest);
  //fclose (log_file_fp);

}

void
zrpc_info(const char *format, ...)
{
  time_t t;
  char buffer[50];
  struct tm* tm_info;
  static char dest[1024];
  va_list argptr;

  if (log_level < ZRPC_LOG_LEVEL_INFORMATIONAL)
    return;

  time (&t);
  tm_info = localtime(&t);
  strftime(buffer, 26, "%Y/%m/%d %H:%M:%S", tm_info);
  va_start(argptr, format);
  vsprintf(dest, format, argptr);
  va_end(argptr);
  if (log_stdout)
    fprintf(stderr, "%s %s ZRPC: %s\r\n",
            buffer, record_priority ?
            zrpc_log_level_str[ZRPC_LOG_LEVEL_INFORMATIONAL] : "", dest);
  if (log_syslog)
    syslog(ZRPC_LOG_LEVEL_INFORMATIONAL | log_facility, "%s", dest);
  if (!log_file_filename)
    return;
  sprintf(dest_sys, "%s %s ZRPC: %s",
            buffer, record_priority ?
            zrpc_log_level_str[ZRPC_LOG_LEVEL_INFORMATIONAL] : "", dest);
  zrpc_system (dest_sys);
}

/* this fuction will configure file logging, and level logging for
 * all target : stdout, file or syslog
 */
void
zrpc_debug_set_log_with_level (char *logFileName, char *logLevel)
{
  log_level = zrpc_level_match(logLevel);
  
  /* close previous file */
  zrpc_debug_flush();

  log_file_filename = ZRPC_STRDUP (logFileName);
}

void
zrpc_debug_configure_syslog (int on)
{
  log_syslog = on;
}

void
zrpc_debug_configure_stdout (int on)
{
  log_stdout = on;
}

void
zrpc_debug_flush (void)
{
  if (log_file_fp)
    fclose (log_file_fp);
  log_file_fp = NULL;
  if (log_file_filename)
    ZRPC_FREE (log_file_filename);
  log_file_filename = NULL;
}
