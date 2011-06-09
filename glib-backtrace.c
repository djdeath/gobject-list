/*
 * glib-backtrace: a LD_PRELOAD library for tracking warnings of GLib
 *
 * Copyright (C) 2011  Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *
 * Authors:
 *     Lionel Landwerlin  <lionel.g.landwerlin@linux.intel.com>
 */
#include <glib-object.h>

#include <dlfcn.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

typedef struct
{
  const gchar *name;
  GLogLevelFlags flag;
} LevelFlagsMapItem;

LevelFlagsMapItem level_flags_map[] =
{
  { "error", G_LOG_LEVEL_ERROR },
  { "critical", G_LOG_LEVEL_CRITICAL },
  { "warning", G_LOG_LEVEL_WARNING },
  { "message", G_LOG_LEVEL_MESSAGE },
  { "info", G_LOG_LEVEL_INFO },
  { "debug", G_LOG_LEVEL_DEBUG },
};

static gboolean
display_level (GLogLevelFlags level)
{
  static GLogLevelFlags level_flags = G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING;
  static gboolean parsed = FALSE;

  if (G_UNLIKELY (!parsed))
    {
      const gchar *filters = g_getenv ("GLIB_LEVELS_FILTER");

      if (filters != NULL)
        {
          gchar **tokens = g_strsplit (filters, ",", 0);
          guint len = g_strv_length (tokens);
          guint i = 0;

          /* If there really are items to parse, clear the default flags */
          if (len > 0)
            level_flags = 0;

          for (; i < len; ++i)
            {
              gchar *token = tokens[i];
              guint j = 0;

              for (; j < G_N_ELEMENTS (level_flags_map); ++j)
                {
                  if (!g_ascii_strcasecmp (token, level_flags_map[j].name))
                    {
                      level_flags |= level_flags_map[j].flag;
                      break;
                    }
                }
            }

          g_strfreev (tokens);
        }

      parsed = TRUE;
    }

  return (level_flags & level) ? TRUE : FALSE;
}

static gboolean
display_domain (const gchar *domain)
{
  static GHashTable *domains = NULL;

  if (G_UNLIKELY (!domains))
    {
      const char *filters = g_getenv ("GLIB_DOMAIN_FILTERS");

      domains = g_hash_table_new (g_str_hash, g_str_equal);

      if (filters)
        {
          gchar **tokens = g_strsplit (filters, ",", 0);

          guint len = g_strv_length (tokens);
          guint i = 0;

          for (i = 0; i < len; i++)
            g_hash_table_insert (domains, tokens[i], GINT_TO_POINTER (TRUE));

          g_strfreev (tokens);
        }
    }

  if ((g_hash_table_size (domains) == 0) ||
      (g_hash_table_lookup (domains, domain) != NULL))
    return TRUE;

  return FALSE;
}

static void
print_trace (void)
{
#ifdef HAVE_LIBUNWIND
  unw_context_t uc;
  unw_cursor_t cursor;
  guint stack_num = 0;

  unw_getcontext (&uc);
  unw_init_local (&cursor, &uc);

  while (unw_step (&cursor) > 0)
    {
      gchar name[65];
      unw_word_t off;

      if (unw_get_proc_name (&cursor, name, 64, &off) < 0)
        {
          g_print ("Error getting proc name\n");
          break;
        }

      g_print ("#%d  %s + [0x%08x]\n", stack_num++, name, (unsigned int)off);
    }
#endif
}

static void *
get_func (const char *func_name)
{
  static void *handle = NULL;
  void *func;
  char *error;

  if (G_UNLIKELY (handle == NULL))
    {
      handle = dlopen("libglib-2.0.so", RTLD_LAZY);

      if (handle == NULL)
        g_error ("Failed to open libglib-2.0.so: %s", dlerror ());
    }

  func = dlsym (handle, func_name);

  if ((error = dlerror ()) != NULL)
    g_error ("Failed to find symbol: %s", error);

  return func;
}

void
g_log (const gchar   *log_domain,
       GLogLevelFlags log_level,
       const gchar   *format,
       ...)
{
  static void (* real_g_logv) (const gchar   *log_domain,
                               GLogLevelFlags log_level,
                               const gchar   *format,
                               va_list	       args1) = NULL;
  va_list args;

  if (G_UNLIKELY (!real_g_logv))
    real_g_logv = get_func ("g_logv");

  va_start (args, format);
  real_g_logv (log_domain, log_level, format, args);
  va_end (args);

  if (display_level (log_level) && display_domain (log_domain))
    print_trace ();
}

