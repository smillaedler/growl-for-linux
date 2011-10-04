#ifndef _gol_h_
#define _gol_h_

#include <glib.h>

#define GOL_PP_CAT_IMPL_(x, y) x ## y
#define GOL_PP_CAT(x, y) GOL_PP_CAT_IMPL_(x, y)

#if defined(__cplusplus)
# define GOL_INLINE inline
#elif defined(__GNUC__)
# define GOL_INLINE __attribute__((unused)) static
#else
# define GOL_INLINE static
#endif

#define GOL_UNUSED_ARG_IMPL_ GOL_PP_CAT(_gol_unused_argument_, __LINE__)
#if defined(__GNUC__)
# define GOL_UNUSED_ARG(n) GOL_PP_CAT(GOL_UNUSED_ARG_IMPL_, GOL_PP_CAT(_, n)) __attribute__((unused))
#else
# define GOL_UNUSED_ARG(n) GOL_PP_CAT(GOL_UNUSED_ARG_IMPL_, GOL_PP_CAT(_, n))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  gchar* title;
  gchar* text;
  gchar* icon;
  gchar* url;
  gboolean local;
} NOTIFICATION_INFO;

typedef struct {
  void (*show)(NOTIFICATION_INFO* ni);
} SUBSCRIPTOR_CONTEXT;

GOL_INLINE void
free_notification_info(NOTIFICATION_INFO* const ni) {
  if (!ni) return;
  g_free(ni->title);
  g_free(ni->text);
  g_free(ni->icon);
  g_free(ni->url);
  g_free(ni);
}

#ifdef __cplusplus
}
#endif

#endif /* _gol_h_ */

